// Copyright 2024 Khalil Estell
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

#include <libhal/spi.hpp>

#include <libhal/error.hpp>

#include <boost/ut.hpp>

namespace hal {
namespace {
constexpr hal::spi::settings expected_settings{
  .clock_rate = 10.0_kHz,
  .clock_idles_high = true,
  .data_valid_on_trailing_edge = true,
};
constexpr hal::spi::settings expected_settings2{
  .clock_rate = 10.0_kHz,
  .cpol = true,
  .cpha = false,
};
class test_spi : public hal::spi
{
public:
  settings m_settings{};
  std::span<hal::byte const> m_data_out{};
  std::span<hal::byte> m_data_in{};
  hal::byte m_filler{};
  bool m_return_error_status{ false };
  ~test_spi() override = default;

private:
  void driver_configure(settings const& p_settings) override
  {
    m_settings = p_settings;
  }

  void driver_transfer(std::span<hal::byte const> p_data_out,
                       std::span<hal::byte> p_data_in,
                       hal::byte p_filler) override
  {
    m_data_out = p_data_out;
    m_data_in = p_data_in;
    m_filler = p_filler;
  }
};
}  // namespace

boost::ut::suite<"spi_test"> spi_test = []() {
  using namespace boost::ut;
  "test"_test = []() {
    // Setup
    test_spi test;
    std::array<hal::byte, 4> const expected_out{ 'a', 'b' };
    std::array<hal::byte, 4> expected_in{ '1', '2' };
    auto const expected_filler = ' ';

    // Exercise
    test.configure(expected_settings);
    test.transfer(expected_out, expected_in, expected_filler);

    // Verify
    expect(that % expected_out.data() == test.m_data_out.data());
    expect(that % expected_in.data() == test.m_data_in.data());
    expect(expected_filler == test.m_filler);
    expect(expected_settings.clock_rate == test.m_settings.clock_rate);
    expect(expected_settings.clock_idles_high ==
           test.m_settings.clock_idles_high);
    expect(expected_settings.data_valid_on_trailing_edge ==
           test.m_settings.data_valid_on_trailing_edge);
    expect(expected_settings.cpol == test.m_settings.cpol);
    expect(expected_settings.cpha == test.m_settings.cpha);
    expect(expected_settings.clock_polarity == test.m_settings.clock_polarity);
    expect(expected_settings.clock_phase == test.m_settings.clock_phase);
  };

  "hal::spi::settings2"_test = []() {
    // Setup
    test_spi test;
    std::array<hal::byte, 4> const expected_out{ 'a', 'b' };
    std::array<hal::byte, 4> expected_in{ '1', '2' };
    auto const expected_filler = ' ';

    // Exercise
    test.configure(expected_settings2);
    test.transfer(expected_out, expected_in, expected_filler);

    // Verify
    expect(that % expected_out.data() == test.m_data_out.data());
    expect(that % expected_in.data() == test.m_data_in.data());
    expect(expected_filler == test.m_filler);
    expect(expected_settings2.clock_rate == test.m_settings.clock_rate);
    expect(expected_settings2.cpol == test.m_settings.cpol);
    expect(expected_settings2.cpha == test.m_settings.cpha);
    expect(expected_settings2.clock_idles_high ==
           test.m_settings.clock_idles_high);
    expect(expected_settings2.data_valid_on_trailing_edge ==
           test.m_settings.data_valid_on_trailing_edge);
    expect(expected_settings2.clock_polarity == test.m_settings.clock_polarity);
    expect(expected_settings2.clock_phase == test.m_settings.clock_phase);
  };
};
}  // namespace hal
