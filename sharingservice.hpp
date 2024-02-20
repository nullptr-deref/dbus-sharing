#pragma once

#include <sdbus-c++/sdbus-c++.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

bool checkExtensionCompatibility(const std::string &filepath);

namespace ipc {
    struct EndpointInfo {
        std::string executablePath;
        // std::string dbusWellKnownName;
        // std::string dbusObjectPath;
        // std::string dbusInterface;
        std::shared_ptr<std::vector<std::string>> acceptedFormats = nullptr;
    };

    using namespace std::literals::string_literals;
    constexpr const char *DEFAULT_CONFIG_PATH = "/etc/sharing/dbus-sharing.conf";

    class SharingService {
    public:
        explicit SharingService(const std::string &configPath = std::string { DEFAULT_CONFIG_PATH });

        std::vector<std::string> getEndpoints() const;
        EndpointInfo getEndpointInfoByName(const std::string &name) const;
        void routeFileToEndpoint(const std::string &endpointName, const std::string &filepath) const;

        void run() const;
    private:
        void launchEndpointService(const std::string &serviceName, const std::string &filepath) const;
        void readConfig(const std::string &configPath);
        std::unordered_map<std::string, EndpointInfo> m_endpoints;
        std::string m_receivedFilepath;
        std::string m_resultPath;

        std::shared_ptr<sdbus::IConnection> m_connection;
    };
}

/*
 * SharingService is a systemd service launched on startup
 * helping other applications to get information about
 * applications providing file sharing.
 * It should somehow get information about executables which
 * are responsible for handling files (how to call them, which
 * filetypes they support, etc.) and send data to this applications.
 * It should contain list of those executables.
 */
