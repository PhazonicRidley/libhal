#pragma once
#include <array>
#include <libhal/units.hpp>
#include <span>
#include <string_view>

// TODO: Potentially move this file to util

namespace hal::experimental {
// Misc
enum class usb_class_code : u8
{
  USE_INTERFACE_DESCRIPTOR =
    0x00,               // Use class information in the Interface Descriptors
  AUDIO = 0x01,         // Audio device class
  CDC_CONTROL = 0x02,   // Communications and CDC Control
  HID = 0x03,           // Human Interface Device
  PHYSICAL = 0x05,      // Physical device class
  IMAGE = 0x06,         // Still Imaging device
  PRINTER = 0x07,       // Printer device
  MASS_STORAGE = 0x08,  // Mass Storage device
  HUB = 0x09,           // Hub device
  CDC_DATA = 0x0A,      // CDC-Data device
  SMART_CARD = 0x0B,    // Smart Card device
  CONTENT_SECURITY = 0x0D,      // Content Security device
  VIDEO = 0x0E,                 // Video device
  PERSONAL_HEALTHCARE = 0x0F,   // Personal Healthcare device
  AUDIO_VIDEO = 0x10,           // Audio/Video Devices
  BILLBOARD = 0x11,             // Billboard Device Class
  USB_C_BRIDGE = 0x12,          // USB Type-C Bridge Class
  BULK_DISPLAY = 0x13,          // USB Bulk Display Protocol Device Class
  MCTP = 0x14,                  // MCTP over USB Protocol Endpoint Device Class
  I3C = 0x3C,                   // I3C Device Class
  DIAGNOSTIC = 0xDC,            // Diagnostic Device
  WIRELESS_CONTROLLER = 0xE0,   // Wireless Controller
  MISC = 0xEF,                  // Miscellaneous
  APPLICATION_SPECIFIC = 0xFE,  // Application Specific
  VENDOR_SPECIFIC = 0xFF        // Vendor Specific
};
// Requests

// Descriptors

/** @brief The bDescriptorType fields for common USB descriptors
 */
enum class descriptor_type : byte
{
  DEVICE = 0x1,
  CONFIGURATION = 0x2,
  STRING = 0x3,
  INTERFACE = 0x4,
  ENDPOINT = 0x5,
  DEVICE_QUALIFIER = 0x6,
  OTHER_SPEED_CONFIGURATION = 0x7,
  INTERFACE_POWER = 0x8,
  OTG = 0x9,
  DEBUG = 0xA,
  INTERFACE_ASSOCIATION = 0xB,
  SECURITY = 0xC,
  KEY = 0xD,
  ENCRYPTION_TYPE = 0xE,
  BOS = 0xF,
  DEVICE_CAPABILITY = 0x10,
  WIRELESS_ENDPOINT_COMPANION = 0x11,
  SUPERSPEED_ENDPOINT_COMPANION = 0x30,
  SUPERSPEED_ENDPOINT_ISOCHRONOUS_COMPANION = 0x31
};

// TODO: maybe nuke?
struct usb_descriptor
{
  constexpr usb_descriptor(byte p_bDescriptionType,
                           std::span<byte> p_data_buf,
                           byte p_length)
    : bDescriptionType(p_bDescriptionType)
    , bLength(p_length)
    , m_data(p_data_buf){};

  /** @brief Packs the struct into the given buffer for descriptor to be sent
   * over the bus
   * @returns A span to the packed descriptor.
   */
  virtual std::span<byte> pack()
  {
    m_data[0] = bLength;
    m_data[1] = bDescriptionType;

    return m_data;
  }

  byte const bDescriptionType;
  byte const bLength;

protected:
  std::span<byte> m_data;
  virtual ~usb_descriptor() = default;
};

// TODO (PhazonicRidley): Generalize and add to functional.hpp
template<class Base>
struct injector : public Base
{
  constexpr injector<Base>(byte p_bDescriptionType,
                           std::span<byte> p_data_buf,
                           byte p_length)
    : Base(p_bDescriptionType, p_data_buf, p_length){};

  std::span<byte> pack() final
  {
    Base::pack();
    return descriptor_pack();
  }

protected:
  virtual std::span<byte> descriptor_pack() = 0;
};

// TODO Maybe nuke?
struct device_descriptor : public injector<usb_descriptor>
{
  constexpr device_descriptor(std::span<byte> p_data_buf,
                              u16 p_bcdUSB,
                              byte p_bDeviceClass,
                              byte p_bDeviceSubClass,
                              byte p_bDeviceProtocol,
                              u16 p_idVendor,
                              u16 p_idProduct,
                              u16 p_bcdDevice,
                              byte p_iManufacturer,
                              byte p_iProduct,
                              byte p_iSerialNumber,
                              byte p_bNumConfigurations)
    : injector<usb_descriptor>(static_cast<byte>(descriptor_type::DEVICE),
                               p_data_buf,
                               18)
    , bcdUSB(p_bcdUSB)
    , bDeviceClass(p_bDeviceClass)
    , bDeviceSubClass(p_bDeviceSubClass)
    , bDeviceProtocol(p_bDeviceProtocol)
    , idVendor(p_idVendor)
    , idProduct(p_idProduct)
    , bcdDevice(p_bcdDevice)
    , iManufacturer(p_iManufacturer)
    , iProduct(p_iProduct)
    , iSerialNumber(p_iSerialNumber)
    , bNumConfigurations(p_bNumConfigurations){
      bMaxPacketSize =  // get from control endpoint.
    };

  std::span<byte> descriptor_pack() override
  {
    size_t offset = m_data.size();  // first two bytes are used.
    // TODO
    return m_data;
  }

  u16 bcdUSB;
  byte bDeviceClass;
  byte bDeviceSubClass;
  byte bDeviceProtocol;
  byte bMaxPacketSize;
  u16 idVendor;
  u16 idProduct;
  u16 bcdDevice;
  byte iManufacturer;
  byte iProduct;
  byte iSerialNumber;
  byte bNumConfigurations;
};

template<size_t buffer_size>
struct string_descriptor
{
  string_descriptor(std::string_view p_str){

  };

  std::array<byte, buffer_size> descriptor_size;
};

}  // namespace hal::experimental
