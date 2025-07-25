// Copyright 2024 - 2025 Khalil Estell and the libhal contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cstddef>
#include <span>

#include "units.hpp"

namespace hal {
/**
 * @deprecated Use `v5::serial` instead for better performance
 * @brief Hardware abstract interface for the serial communication protocol
 *
 * Use this interface for hardware that implements a serial protocol like UART,
 * RS232, RS485 and others that use a similar communication protocol but may use
 * different voltage schemes.
 *
 * This interface only works 8-bit serial data frames.
 *
 * Due to the asynchronous and unformatted nature of serial communication
 * protocols, all implementations of serial devices must be buffered. Buffered,
 * in this case, is defined as automatic storage of received bytes without
 * direct application intervention.
 *
 * All implementations MUST allow the user to supply their own buffer of
 * arbitrary size up to the limits of what hardware can support. This allows a
 * developer the ability to tailored the buffer size to the needs of the
 * application.
 *
 * Examples of buffering schemes are:
 *
 * - Using DMA to copy data from a serial peripheral to a region of memory
 * - Using interrupts when a serial peripheral's queue has filled to a point.
 *   Refrain from using interrupts if the peripheral's byte queue is only of
 *   size 1. This is bad for runtime performance and can result in missed bytes.
 *
 */
class serial
{
public:
  /// Generic settings for a standard serial device.
  struct settings
  {
    /// Set of available stop bits options
    enum class stop_bits : uint8_t
    {
      one = 0,
      two,
    };

    /// Set of parity bit options
    enum class parity : uint8_t
    {
      /// Disable parity bit as part of the frame
      none = 0,
      /// Enable parity and set 1 (HIGH) when the number of bits is odd
      odd,
      /// Enable parity and set 1 (HIGH) when the number of bits is even
      even,
      /// Enable parity bit and always return 1 (HIGH) for ever frame
      forced1,
      /// Enable parity bit and always return 0 (LOW) for ever frame
      forced0,
    };

    /// The operating speed of the baud rate (in units of bits per second)
    hertz baud_rate = 115200.0f;

    /// Number of stop bits for each frame
    stop_bits stop = stop_bits::one;

    /// Parity bit type for each frame
    parity parity = parity::none;

    /**
     * @brief Enables default comparison
     *
     */
    bool operator<=>(settings const&) const = default;
  };

  /// Return type for serial read operations
  struct read_t
  {
    /**
     * @brief The filled portion of the input buffer from the serial port
     *
     * The size of this buffer indicates the number of bytes read The address
     * points to the start of the buffer passed into the read() function.
     */
    std::span<byte> data;

    /**
     * @brief Number of enqueued and available to be read out bytes
     *
     * This value can be equal to or exceed the value of capacity. In this
     * situation, the number of bytes above the capacity are bytes that have
     * been dropped. Not all drivers will indicate the number of bytes lost. It
     * is up to the driver or application to decide what to do in this
     * situation.
     */
    size_t available;

    /// The maximum number of bytes that the serial port can queue up.
    size_t capacity;
  };

  /// Return type for serial write operations
  struct write_t
  {
    /// The portion of the buffer transmitted
    std::span<byte const> data;
  };

  /**
   * @brief Configure serial to match the settings supplied
   *
   * Implementing drivers must verify if the settings can be applied to hardware
   * before modifying the hardware. This will ensure that if this operation
   * fails, the state of the serial device has not changed.
   *
   * @param p_settings - settings to apply to serial driver
   * @throws hal::operation_not_supported - if the settings could not be
   * achieved.
   */
  void configure(settings const& p_settings)
  {
    driver_configure(p_settings);
  }

  /**
   * @brief Write data to the transmitter line of the serial port
   *
   * @param p_data - data to be transmitted over the serial port
   * @return write_t - serial write response
   */
  write_t write(std::span<byte const> p_data)
  {
    return driver_write(p_data);
  }

  /**
   * @brief Copy bytes from working buffer into passed buffer
   *
   * This operation copies the bytes from the serial driver's internal working
   * buffer to the buffer supplied.
   *
   * The buffer will be filled up either to the end of the buffer or until there
   * are no more bytes left in the working buffer. The remaining portion of the
   * input buffer is returned in `read_t::remaining`.
   *
   * If a frame error has occurred at any point during serial reception, this
   * function will throw a `hal::io_error` value. The contents of the
   * internal working buffer will stay the same. No information from the
   * internal working buffer will be copied into the supplied buffer and no data
   * will be removed from the internal working buffer. The frame error status
   * will be internally cleared after its occurrence. Subsequent calls of this
   * function will read out the contents of the buffer although the data inside
   * may be corrupt.
   *
   * When an hal::io_error occurs the options available are to flush the buffer
   * and attempt reception again or read out the potentially corrupted data and
   * parse it as needed. The choice of operation is application/driver specific.
   *
   * @param p_data - Buffer to read bytes in to
   * @return read_t - serial read response data
   * @throws hal::io_error - if a frame error occurred at any point during
   * reception of the bytes held currently.
   */
  [[nodiscard]] read_t read(std::span<byte> p_data)
  {
    return driver_read(p_data);
  }

  /**
   * @brief Flush working buffer
   *
   * The behavior of flushing the internal working buffer is this:
   *
   * - Sets the serial port's internal working buffer to an "empty" state.
   * - Clear any received data stored in hardware registers.
   * - Use the fastest available option to perform these operations, meaning
   *   that the contents of the internal working buffer will not be zeroed out.
   */
  void flush()
  {
    driver_flush();
  }

  virtual ~serial() = default;

private:
  virtual void driver_configure(settings const& p_settings) = 0;
  virtual write_t driver_write(std::span<byte const> p_data) = 0;
  virtual read_t driver_read(std::span<byte> p_data) = 0;
  virtual void driver_flush() = 0;
};
}  // namespace hal

namespace hal::v5 {
/**
 * @brief Hardware abstract interface for the serial communication protocol
 *
 * Use this interface for hardware that implements a serial protocol like UART,
 * RS232, RS485 and others that use a similar communication protocol but may use
 * different voltage schemes.
 *
 * This interface only works 8-bit serial data frames.
 *
 * Due to the asynchronous and unformatted nature of serial communication
 * protocols, all implementations of serial devices must be buffered. Buffered,
 * in this case, is defined as automatic storage of received bytes without
 * direct application intervention.
 *
 * All implementations MUST allow the user to supply their own buffer of
 * arbitrary size up to the limits of what hardware can support. This allows a
 * developer the ability to tailored the buffer size to the needs of the
 * application.
 *
 * Examples of buffering schemes are:
 *
 * - Using DMA to copy data from a serial peripheral to a region of memory
 * - Using interrupts when a serial peripheral's queue has filled to a point.
 *   Refrain from using interrupts if the peripheral's byte queue is only of
 *   size 1. This is bad for runtime performance and can result in missed bytes.
 *
 */
class serial
{
public:
  /// Generic settings for a standard serial device.
  struct settings
  {
    /// Set of available stop bits options
    enum class stop_bits : u8
    {
      one = 0,
      two,
    };

    /// Set of parity bit options
    enum class parity : u8
    {
      /// Disable parity bit as part of the frame
      none = 0,
      /// Enable parity and set 1 (HIGH) when the number of bits is odd
      odd,
      /// Enable parity and set 1 (HIGH) when the number of bits is even
      even,
      /// Enable parity bit and always return 1 (HIGH) for ever frame
      forced1,
      /// Enable parity bit and always return 0 (LOW) for ever frame
      forced0,
    };

    /// The operating speed of the baud rate (in units of bits per second)
    hertz baud_rate = 115200;

    /// Number of stop bits for each frame
    stop_bits stop = stop_bits::one;

    /// Parity bit type for each frame
    parity parity = parity::none;

    /**
     * @brief Enables default comparison
     *
     */
    bool operator<=>(settings const&) const = default;
  };
  /**
   * @brief Configure serial to match the settings supplied
   *
   * Implementing drivers must verify if the settings can be applied to hardware
   * before modifying the hardware. This will ensure that if this operation
   * fails, the state of the serial device has not changed.
   *
   * @param p_settings - settings to apply to serial driver
   * @throws hal::operation_not_supported - if the settings could not be
   * achieved.
   */
  void configure(settings const& p_settings)
  {
    driver_configure(p_settings);
  }

  /**
   * @brief Write data to the transmitter line of the serial port
   *
   * @param p_data - data to be transmitted over the serial port
   */
  void write(std::span<hal::byte const> p_data)
  {
    driver_write(p_data);
  }

  /**
   * @brief Returns this serial driver's receive buffer
   *
   * Use this along with the receive_cursor() in order to determine if new data
   * has been read into the receive buffer. See the docs for `receive_cursor()`
   * for more details.
   *
   * @return std::span<hal::byte const> - a const span to the receive buffer
   * used by the serial port. Calling `size()` on the span will always
   * return a value of at least 1.
   */
  [[nodiscard]] std::span<hal::byte const> receive_buffer()
  {
    return driver_receive_buffer();
  }

  /**
   * @brief Returns the current write position of the circular receive buffer
   *
   * Receive cursor represents the position where the next byte of data will be
   * written in to the receive buffer. This position advances as new data
   * arrives. To determine how much new data has arrived, store the previous
   * cursor position and compare it with the current cursor position, accounting
   * for buffer wraparound.
   *
   * The cursor value will ALWAYS follow this equation:
   *
   *          0 <= cursor && cursor < receive_buffer().size()
   *
   * Thus making the following expression valid memory access:
   *
   *         serial.receive_buffer()[ serial.cursor() ];
   *
   * Just note that just because it is valid does not mean that there is useful
   * information at this position.
   *
   * Example:
   *
   *   auto old_head = port.receive_cursor();
   *   // ... wait for new data ...
   *   auto new_head = port.receive_cursor();
   *   // Account for circular wraparound when calculating bytes received
   *   auto buffer_size = port.receive_buffer().size();
   *   auto bytes_received = (new_head + buffer_size - old_head) % buffer_size;
   *
   * Use this along with receive_buffer() to access newly received data. The
   * data between your last saved position and the current cursor position
   * represents the newly received bytes. When reading the data, remember that
   * it may wrap around from the end of the buffer back to the beginning.
   *
   * @return usize - position of the write cursor for the circular buffer
   */
  [[nodiscard]] usize receive_cursor()
  {
    return driver_cursor();
  }

  virtual ~serial() = default;

private:
  virtual void driver_configure(settings const& p_settings) = 0;
  virtual void driver_write(std::span<hal::byte const> p_data) = 0;
  virtual std::span<hal::byte const> driver_receive_buffer() = 0;
  virtual std::size_t driver_cursor() = 0;
};
}  // namespace hal::v5
