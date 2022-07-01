#include <array>
#include <cassert>
#include <charconv>
#include <codecvt>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <locale>
#include <memory>
#include <optional>
#include <vector>

#include <hidapi/hidapi.h>

// Brightness steps for Apple StudioDisplay (2022) on macOS
constexpr auto steps = std::array{
    400, 1424, 2395, 3566, 4985, 6693, 8733, 11152, 14000,
    17331, 21019, 25689, 30854, 36778, 43547, 51254, 60000
};

struct HIDInitializer
{
    HIDInitializer() { hid_init(); };
    ~HIDInitializer() { hid_exit(); };
};

struct Display
{
    uint16_t vendorID;
    uint16_t productID;
    uint16_t minBrightness;
    uint16_t maxBrightness;
};

constexpr Display kAppleStudioDisplay{
    .vendorID = 0x05ac, .productID = 0x1114, .minBrightness = 400, .maxBrightness = 60000};

using DevicePtr = std::unique_ptr<hid_device, decltype(&hid_close)>;
using EnumerationPtr = std::unique_ptr<hid_device_info, decltype(&hid_free_enumeration)>;

void print_device_info(const hid_device_info* device_info)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    std::cout << "manufacturer: " << converter.to_bytes(device_info->manufacturer_string) << '\n';
    std::cout << "product:      " << converter.to_bytes(device_info->product_string) << '\n';
    std::cout << "serial:       " << converter.to_bytes(device_info->serial_number) << '\n';
    std::cout << "release:      " << device_info->release_number << '\n';
    std::cout << "interface:    " << device_info->interface_number << '\n';
    std::cout << "usage page:   " << device_info->usage_page << '\n';
    std::cout << "usage:        " << device_info->usage << '\n';
}

std::optional<std::string> find_device_path(uint16_t vendorID, uint16_t productID, int interfaceID)
{
    EnumerationPtr enumerate_result{hid_enumerate(vendorID, productID), &hid_free_enumeration};
    if (!enumerate_result) {
        return {};
    }
    char* path = nullptr;
    auto* device_info = enumerate_result.get();
    while (device_info) {
        // print_device_info(device_info);
        // std::cout << '\n';

        if (device_info->interface_number == interfaceID) {
            path = device_info->path;
            break;
        }

        device_info = device_info->next;
    }

    std::optional<std::string> ret;
    if (path != nullptr) {
        ret = std::string{path};
    }
    return ret;
}

std::optional<uint16_t> get_brightness(DevicePtr& device)
{
    std::array<u_char, 7> buffer{};
    buffer[0] = 0x1;
    const auto ret = hid_get_feature_report(device.get(), buffer.data(), buffer.size());
    if (ret == -1) {
        return {};
    }
    return static_cast<uint16_t>(buffer[1]) + (static_cast<uint16_t>(buffer[2]) << 8);
}

bool set_brightness(DevicePtr& device, uint16_t val)
{
    assert(val >= kAppleStudioDisplay.minBrightness && val <= kAppleStudioDisplay.maxBrightness);
    std::array<u_char, 7> buffer{0x01,
                                 static_cast<u_char>(val & 0x00ff),
                                 static_cast<u_char>((val >> 8) & 0x00ff),
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00};
    const auto ret = hid_send_feature_report(device.get(), buffer.data(), buffer.size());
    return ret > -1;
}

std::optional<uint16_t> next_value(auto begin, auto end, auto doneFn)
{
    for (; begin != end; ++begin) {
        if (doneFn(*begin)) {
            return *begin;
        }
    }

    return {};
}

std::optional<uint16_t> next_brightness(uint16_t value)
{
    return next_value(steps.cbegin(), steps.cend(), [&](const auto step) { return step > value; });
}

std::optional<uint16_t> prev_brightness(uint16_t value)
{
    return next_value(
        steps.crbegin(), steps.crend(), [&](const auto step) { return step < value; });
}

int main(int argc, char* argv[])
{
    HIDInitializer init{};

    const auto path = find_device_path(kAppleStudioDisplay.vendorID,
                                       kAppleStudioDisplay.productID,
                                       7 /* 7 on Linux/Windows, 12 on macOS */);
    if (!path) {
        std::cout << "Could not find matching device/interface.\n";
        return EXIT_FAILURE;
    }
    DevicePtr device{hid_open_path(path->c_str()), &hid_close};
    if (!device) {
        std::cout << "Could not open device.\n";
        return EXIT_FAILURE;
    }

    const auto brightness = get_brightness(device);
    if (!brightness) {
        std::cout << "Could not read current brightness.\n";
        return EXIT_FAILURE;
    }

    if (argc < 2) {
        std::cout << "current brightness: " << *brightness << '\n';
    } else {
        const auto arg = std::string_view{argv[1]};
        if (arg.starts_with("--inc")) {
            if (const auto nextBrightnessStep = next_brightness(*brightness); nextBrightnessStep) {
                set_brightness(device, *nextBrightnessStep);
            }
        } else if (arg.starts_with("--dec")) {
            if (const auto prevBrightnessStep = prev_brightness(*brightness); prevBrightnessStep) {
                set_brightness(device, *prevBrightnessStep);
            }
        } else {
            std::cout << "Usage: " << argv[0] << "[--increase|--decrease]\n";
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
