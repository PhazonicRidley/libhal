#pragma once
#include "data_structures.hpp"
#include "endpoints.hpp"
#include "libhal/units.hpp"
#include <array>
#include <libhal/error.hpp>
#include <libhal/functional.hpp>
#include <span>
#include <utility>

namespace hal::experimental {
// Potentially will need to turn stored data to be RW instead of just RO

// TODO: Create endpoint iterator

class usb_interface
{
public:
  struct interface_settings
  {
    friend usb_interface;
    interface_settings(u8 p_num_endpoints,
                       usb_class_code p_class,
                       u8 p_subclass,
                       u8 p_protocol,
                       u8 p_str_name_idx)
      : m_num_endpoints(p_num_endpoints)
      , m_subclass(p_subclass)
      , m_protocol(p_protocol)
      , m_str_name_idx(p_str_name_idx)
    {
      if (p_class == usb_class_code::USE_INTERFACE_DESCRIPTOR ||
          p_class == usb_class_code::HUB ||
          p_class == usb_class_code::BILLBOARD) {
        safe_throw(hal::argument_out_of_domain(this));
      }
      m_class = p_class;
    };

  private:
    u8 m_num_endpoints;
    usb_class_code m_class;
    u8 m_subclass;
    u8 m_protocol;
    u8 m_str_name_idx;
  };
  // Write out descriptors for a specific interface, writes out all child
  // descriptors as well
  // TODO: Migrate to a flat_map for settings
  // Can throw, doesn't handle exceptions here
  usb_interface(u8 p_iface_number,
                usb_control_endpoint& p_ctrl_endpoint,
                std::span<std::pair<u8, interface_settings>> p_settings)
    : m_settings(p_settings)
    , m_ctrl_endpoint(p_ctrl_endpoint)
  {
    m_packed_data[0] = 0x9;
    m_packed_data[1] = static_cast<u8>(descriptor_type::INTERFACE);
    m_packed_data[2] = p_iface_number;
    m_packed_data[3] = 0;  // Default config is zero
    set_setting(0);
  }

  virtual void write_descriptors(
    callback<void(std::span<byte const>)> p_dispatch) = 0;
  virtual size_t total_length() = 0;
  virtual ~usb_interface() = default;

  virtual usb_endpoint& acquire_endpoint(hal::u8 p_index) = 0;

  // TODO: Finish business logic as according to the USB protocol
  void set_setting(u8 p_number)
  {
    // This method really won't be used, just putting scaffolding
    auto& iface_setting = get_interface_setting(p_number);
    m_packed_data[4] = iface_setting.m_num_endpoints;
    m_packed_data[5] = static_cast<u8>(iface_setting.m_class);
    m_packed_data[6] = iface_setting.m_subclass;
    m_packed_data[7] = iface_setting.m_protocol;
    m_packed_data[8] = iface_setting.m_str_name_idx;
    m_selected_setting_number = p_number;
    // Write out descriptor via ctrl endpoint
  }

  inline u8 get_interface_number()
  {
    return m_packed_data[2];
  }

  inline u8 get_selected_setting_number()
  {
    return m_selected_setting_number;
  }

  // Can throw
  interface_settings& get_interface_setting(u8 p_number)
  {
    for (auto& p : m_settings) {
      if (p.first == p_number) {
        return p.second;
      }
    }

    safe_throw(hal::argument_out_of_domain(this));
  }

  // TODO: Discuss with Khalil about ctrl endpoint class tracking its own buffer
  void ctrl_write(std::span<byte const> p_buffer);

  // Buffer to collect data out of the endpoint
  std::span<byte const> ctrl_read(std::span<u8> p_buffer);

private:
  std::array<byte, 9> m_packed_data;
  std::span<std::pair<u8, interface_settings>> m_settings;
  u8 m_selected_setting_number;
  usb_control_endpoint& m_ctrl_endpoint;
  // string descriptor
};

// TODO find a home for this function
std::array<byte, 2> pack_u16_le(u16 p_dat)
{
  return std::array<byte, 2>{ static_cast<byte>(p_dat & 0xFF),
                              static_cast<byte>((p_dat & (0xFF << 8)) >> 8) };
}

// Potentially will need to turn stored data to be RW instead of just RO
// Right now, we pack immediately and then give the user options to query the
// data. RW would mean we'd need a manual repack method to be called on
// reenumeration
class usb_configuration
{
public:
  // This could potentially be a standalone function and this struct is used
  // exclusively for housing interfaces.
  usb_configuration(std::span<usb_interface*> p_ifaces,
                    bool p_self_powered,
                    bool p_remote_wakeup,
                    u8 p_max_power)
    : m_ifaces(p_ifaces)
  {
    m_packed_data[0] = m_packed_data.size();
    m_packed_data[1] = static_cast<u8>(descriptor_type::CONFIGURATION);
    u16 wTotalLength = m_packed_data.size();
    for (auto iface : m_ifaces) {
      wTotalLength += iface->total_length();
    }
    auto packed_total_length = pack_u16_le(wTotalLength);
    m_packed_data[2] = packed_total_length[0];
    m_packed_data[3] = packed_total_length[1];
    m_packed_data[4] = m_ifaces.size();
    // For config value (and maybe the string problem):
    // When descriptor(s) is about to be written out, enumerator will...
    // enumerate configs and it will be determined by p_dispatch on how to
    // number them The simplist solution is, p_dispatch is a capture within the
    // core enumerate function, which has a counter for configurations seen.

    // TODO: resolve the string problem (see above maybe for that to work, there
    // would need to be a seperate string enumeration function for all data
    // structures)
    u8 bmAttributes = 0x80 | (p_self_powered << 6) | (p_remote_wakeup << 5);
    m_packed_data[7] = bmAttributes;
    m_packed_data[8] = p_max_power;
  }

  inline u16 get_total_length()
  {
    return static_cast<u16>(m_packed_data[3] << 8) |
           static_cast<u16>(m_packed_data[2]);
  }

  inline u8 get_interface_count()
  {
    return m_packed_data[4];
  }

  inline u8 get_number()
  {
    return m_packed_data[5];
  }

  // TODO: reorganize
  struct config_attributes
  {
    bool self_powered;
    bool remote_wakeup;
  };

  inline config_attributes get_attributes()
  {
    u8 bm = m_packed_data[7];
    return config_attributes{ .self_powered = (bm & (1 << 6)) != 0,
                              .remote_wakeup = (bm & (1 << 5)) != 0 };
  }

  inline u8 get_max_power()
  {
    return m_packed_data[8];
  }

  // Write out config descriptor and children descriptors
  // Maybe return a span or an array of the data written?
  // Maybe make this virtual? I'm not sure how much people will want to
  // inherit this
  // p_dispatch should have access to the control endpoint
  void write_descriptors(callback<void(std::span<byte const>)> p_dispatch) const
  {
    p_dispatch(m_packed_data);
    // TODO: Maybe put this responsiblity onto the enumerator (or use visitation
    // pattern)
    for (auto iface : m_ifaces) {
      iface->write_descriptors(p_dispatch);
    }
  }

private:
  // config value given by enumerator, maybe have a variable for this for use
  // outside our enumerator?
  // TODO string descriptors
  std::span<usb_interface*> m_ifaces;
  std::array<byte, 9> m_packed_data;
};

}  // namespace hal::experimental
