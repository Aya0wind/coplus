//
// Created by junjian LI on 2023/4/25.
//

#pragma once
#include <arpa/inet.h>
#include <cstdint>
#include <string>
namespace coplus {

    class ipv4 {
        uint32_t ip_;

    public:
        explicit ipv4(const std::string& ip) {
            ip_ = inet_addr(ip.data());
        }
        ipv4(int p1, int p2, int p3, int p4) {
            ip_ = (p1 << 24) | (p2 << 16) | (p3 << 8) | p4;
        }

        ipv4(const ipv4& other) :
            ip_(other.ip_) {
        }
        ipv4& operator=(const ipv4& other) noexcept = default;
        ipv4(ipv4&& other) noexcept = default;
        ipv4& operator=(ipv4&& other) noexcept = default;
        int32_t bin() const {
            return ip_;
        }
        void set_ip(int32_t ip) {
            ip_ = ip;
        }
        void set_ip(int p1, int p2, int p3, int p4) {
            ip_ = (p1 << 24) | (p2 << 16) | (p3 << 8) | p4;
        }
        [[nodiscard]] std::string to_string() const {
            std::string res = std::to_string((ip_ >> 24) & 0xFF);
            res += ".";
            res += std::to_string((ip_ >> 16) & 0xFF);
            res += ".";
            res += std::to_string((ip_ >> 8) & 0xFF);
            res += ".";
            res += std::to_string(ip_ & 0xFF);
            return res;
        }
    };
    template<class IP>
    class net_address {
        IP ip_;
        uint16_t port_;

    public:
        net_address(const IP& ip, uint16_t port) :
            ip_(ip), port_(port) {
        }
        net_address(const net_address& other) = default;
        net_address& operator=(const net_address& other) = default;
        net_address(net_address&& other) = default;
        net_address& operator=(net_address&& other) noexcept = default;
        net_address(int p1, int p2, int p3, int p4, uint16_t port) :
            ip_(p1, p2, p3, p4), port_(port) {
        }

        void set_port(uint16_t port) {
            port_ = port;
        }

        const IP& ip() const {
            return ip_;
        }
        uint16_t port() const {
            return port_;
        }
    };
}// namespace coplus

template<class IP>
[[nodiscard]] std::string to_string(const coplus::net_address<IP>& addr) {
    return std::to_string(addr.ip()) + ":" + std::to_string(addr.port());
}
