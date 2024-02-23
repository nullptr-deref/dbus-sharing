#include "sharing.hpp"

#include <sdbus-c++/sdbus-c++.h>

#include <systemd/sd-journal.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <stdio.h>

constexpr char LIST_SEP = ',';

bool checkExtensionCompatibility(const ipc::EndpointInfo &info, const std::string &filepath) {
    // TODO: add handling files with no extension
    const std::string actualFmt = filepath.substr(filepath.rfind('.'));
    return std::find(info.acceptedFormats->cbegin(), info.acceptedFormats->cend(), actualFmt) != info.acceptedFormats->cend();
}

ipc::SharingService::SharingService(const std::string &configPath) {
    this->readConfig(configPath);
    constexpr const char *sharingConnectionName = "org.rt.sharing";
    m_connection = sdbus::createSessionBusConnection(sharingConnectionName);

    constexpr const char *objPath = "/org/rt/router";
    constexpr const char *interfaceName = "org.rt.SharingService";
    auto router = sdbus::createObject(*m_connection, objPath);
    auto routeFunc = [this](const std::string &endpointName, const std::string &filepath) -> void {
        this->routeFileToEndpoint(endpointName, filepath);
    };
    auto getEndpointsFunc = [this, &router]() {
        const auto endpointsList = this->getEndpoints();
        router->emitSignal("endpointsReady").onInterface(interfaceName).withArguments(endpointsList);
        return endpointsList;
    };
    auto getEndpointFormats = [this](const std::string &endpointName) -> std::vector<std::string> {
        return *this->getEndpointInfoByName(endpointName).acceptedFormats;
    };
    router->registerMethod("getEndpoints").onInterface(interfaceName).implementedAs(std::move(getEndpointsFunc));
    router->registerMethod("passFileForProcessing").onInterface(interfaceName).implementedAs(std::move(routeFunc));
    router->registerMethod("getEndpointFormats").onInterface(interfaceName).implementedAs(std::move(getEndpointFormats));
    router->registerSignal("endpointsReady").onInterface(interfaceName).withParameters<std::vector<std::string>>();
    router->finishRegistration();

    m_connection->enterEventLoop();
}

void ipc::SharingService::run() const {
    m_connection->enterEventLoop();
}

void ipc::SharingService::routeFileToEndpoint(const std::string &endpointName, const std::string &filepath) const {
    this->launchEndpointService(endpointName, filepath);
}

void ipc::SharingService::readConfig(const std::string &configPath) {
    std::ifstream configFile { configPath };
    if (!configFile.is_open()) {
        char errMsg[256];
        ::sprintf(errMsg, "File sharing proxy service could not open file %s: no such file or directory.", configPath.c_str());
        ::sd_journal_print(LOG_CRIT, errMsg);
        std::cerr << errMsg << '\n';
        throw std::runtime_error("errMsg"); // indirect call of std::terminate()
    }
    std::string line;
    std::string appName = "";
    while (std::getline(configFile, line)) {
        if (line.find('[') != std::string::npos) {
            const std::size_t nameBegin = line.find('[') + 1;
            const std::size_t nameLen = line.find(']') - nameBegin;
            appName = line.substr(nameBegin, nameLen);
            m_endpoints.insert_or_assign(appName, EndpointInfo{});
            m_endpoints.at(appName).name = appName;
            continue;
        }
        if (line.starts_with("formats")) {
            if (!m_endpoints[appName].acceptedFormats) {
                m_endpoints[appName].acceptedFormats = std::make_shared<std::vector<std::string>>();
            }
            const std::size_t listBegin = line.find('=') + 1;
            std::string_view formatsList { line.substr(listBegin) };
            while (formatsList.size() > 0) {
                const std::size_t nextSepIndex = formatsList.find(LIST_SEP);
                if (nextSepIndex < 1) continue;
                if (nextSepIndex != std::string::npos) {
                    m_endpoints[appName].acceptedFormats->emplace_back(formatsList.substr(0, nextSepIndex));
                    formatsList.remove_prefix(nextSepIndex + 1);
                } else {
                    m_endpoints[appName].acceptedFormats->emplace_back(formatsList.substr(0, formatsList.size()));
                    break;
                }
            }
        }
        if (line.starts_with("cmd")) {
            m_endpoints[appName].executablePath = line.substr(line.find('=') + 1);
        }
    }
}

std::vector<std::string> ipc::SharingService::getEndpoints() const {
    std::vector<std::string> res;
    for (const auto &[endpointName, _] : m_endpoints) {
        res.push_back(endpointName);
    }

    return res;
}

ipc::EndpointInfo ipc::SharingService::getEndpointInfoByName(const std::string &name) const {
    return m_endpoints.at(name);
}

void ipc::SharingService::launchEndpointService(const std::string &serviceName, const std::string &filepath) const {
    const auto endpoint = m_endpoints.at(serviceName);
    const auto &endpointExecPath = m_endpoints.at(serviceName).executablePath;
    if (checkExtensionCompatibility(endpoint, filepath)) {
        ::execl(endpoint.executablePath.c_str(), endpoint.executablePath.c_str(), filepath.c_str(), nullptr);
    }
}
