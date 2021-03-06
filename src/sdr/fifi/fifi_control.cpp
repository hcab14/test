// -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil  -*-
// $Id$
//
// Copyright 2010-2014 Christoph Mayer
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <vector>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include "sdr/fifi/fifi_control.hpp"

namespace FiFiSDR {  
  class receiver_control_impl : public receiver_control {
  public:
    receiver_control_impl(usb_device_handle::sptr usb_device)
      : usb_control_(usb_control::make(usb_device))
      , have_abpf_(false)
      , abpf_(256,0) {
      get_abpf();
    }
    virtual ~receiver_control_impl() {}

    virtual void init(const boost::property_tree::ptree& config) {
      LOG_INFO(str(boost::format("firmware svn version_num=%d version_str='%s'")
                   % get_version_number() 
                   % get_version_string()));
      
      const double harmonic3rd_MHz(config.get<double>("<xmlattr>.harmonic3rd_MHz", 35.));
      const double freq_3rd(get_nth_frequency(3));
      if (freq_3rd != harmonic3rd_MHz) {
        set_nth_frequency(3, harmonic3rd_MHz);
        LOG_INFO(str(boost::format("3rd harmonic frequency old=%fMHz changed to %fMHz") 
                     % freq_3rd % harmonic3rd_MHz));
      } else {
        LOG_INFO(str(boost::format("3rd harmonic frequency old=%fMHz not changed") 
                     % freq_3rd));    
      }
      
      const double harmonic5th_MHz(config.get<double>("<xmlattr>.harmonic5th_MHz", 85.));
      const double freq_5th(get_nth_frequency(5));
      if (freq_5th != harmonic5th_MHz) {
        set_nth_frequency(5, harmonic5th_MHz);
        LOG_INFO(str(boost::format("5th harmonic frequency old=%fMHz changed to %fMHz)") 
                     % freq_5th % harmonic5th_MHz));
      } else {
        LOG_INFO(str(boost::format("5th harmonic frequency old=%fMHz not changed") 
                     % freq_5th));    
      }
      
      const boost::property_tree::ptree& config_presel(config.get_child("Preselector"));
      const boost::uint32_t presel_mode(config_presel.get<boost::uint32_t>("<xmlattr>.mode", 1));
      const boost::uint32_t pmode(get_presel_mode());
      if (pmode != presel_mode) {
        set_presel_mode(presel_mode);
        LOG_INFO(str(boost::format("preselector mode old=%d changed to %d") 
                     % pmode % presel_mode));
      } else {
        LOG_INFO(str(boost::format("preselector mode old=%d not changed") 
                     % pmode));
      }
      
      int counter(-1);
      for (auto const& entry : config_presel) {
        if (entry.first != "Entry") {
          if (entry.first != "<xmlattr>")
            LOG_WARNING(str(boost::format("tag '%s' not supported: expect 'Entry'") % entry.first));
          continue;
        }
        const size_t
          index(entry.second.get<size_t>("<xmlattr>.index",         counter++));
        const receiver_control::presel_entry pe(get_presel_entry(index));
        const receiver_control::presel_entry
          pentry(entry.second.get<double>("<xmlattr>.freqFrom_MHz", 0.),
                 entry.second.get<double>("<xmlattr>.freqTo_MHz",   0.),
                 entry.second.get<size_t>("<xmlattr>.pattern",      0));           
        if (pentry == pe) {
          LOG_INFO(str(boost::format("preselector entry(%02d) old=%s not changed")
                       % index % pe));
        } else {
          set_presel_entry(index, pentry);
          LOG_INFO(str(boost::format("preselector entry(%02d) old=%s changed to %s")
                       % index % pe % pentry));
          counter = index;
        }
      }
      // set default entry
      if (counter >= 0 && counter != 15) {
        const receiver_control::presel_entry pentry(0.0, 500.0, 0);
        set_presel_entry(15, pentry);
        LOG_INFO(str(boost::format("preselector entry(15) changed to %s") % pentry));
      }      
    }
    
    virtual boost::uint32_t get_version_number() const {
      boost::uint32_t version_num(0);
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0xAB, 0, 0, (unsigned char *)&version_num, 4) == 4);
      return version_num;
    }
    virtual std::string get_version_string() const {
      const size_t len(256);
      char version[len+1];
      memset(version, 0, sizeof(version));
      const ssize_t n(usb_control_->submit_control(request_type_in(), 0xAB, 0, 1, (unsigned char *)&version, len));
      ASSERT_THROW(n >= 0);
      ASSERT_THROW(size_t(n) <= len);
      return std::string(version);
    }    
    virtual double get_frequency() const { 
      boost::uint32_t freq21(0);
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0x3A, 0, 0, (unsigned char *)&freq21, 4) == 4);
      return c2d<21>(freq21)/4.;
    }
    virtual void set_frequency(double freq) {
      const boost::uint32_t freq21(d2c<21>(4.*freq));
      ASSERT_THROW(usb_control_->submit_control(request_type_out(), 0x32, 0, 0, (unsigned char *)&freq21, 4) == 4);
    }
    virtual double get_startup_frequency() const { 
      boost::uint32_t freq21(0);
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0x3C, 0, 0, (unsigned char *)&freq21, 4) == 4);
      return c2d<21>(freq21)/4.;
    }
    virtual void set_startup_frequency(double freq) {
      const boost::uint32_t freq21(d2c<21>(4.*freq));
      ASSERT_THROW(usb_control_->submit_control(request_type_out(), 0x34, 0, 0, (unsigned char *)&freq21, 4) == 4);
    }

    virtual double get_xtal_frequency() const {
      boost::uint32_t freq24(0);
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0x3D, 0, 0, (unsigned char *)&freq24, 4) == 4);
      return c2d<24>(freq24);
    }
    virtual void set_xtal_frequency(double freq) {
      boost::uint32_t freq24(d2c<24>(freq));
      ASSERT_THROW(usb_control_->submit_control(request_type_out(), 0x33, 0, 0, (unsigned char *)&freq24, 4) == 4);
    }

    virtual double get_nth_frequency(unsigned n) const {
      boost::uint32_t freq21(0);
      ASSERT_THROW(n == 3 || n==5);
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0xAB, 0, n, (unsigned char *)&freq21, 4) == 4);
      return c2d<21>(freq21)/4.;      
    }
    virtual void set_nth_frequency(unsigned n, double freq) {
      const boost::uint32_t freq21(d2c<21>(4.*freq));
      ASSERT_THROW(n == 3 || n==5);
      ASSERT_THROW(usb_control_->submit_control(request_type_out(), 0xAC, 0, n, (unsigned char *)&freq21, 4) == 4);
    }

    virtual boost::uint32_t get_presel_mode() const {
      boost::uint32_t mode(0);
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0xAB, 0, 6, (unsigned char *)&mode, 4) == 4);
      return mode;
    }
    virtual void set_presel_mode(boost::uint32_t mode) {
      ASSERT_THROW(usb_control_->submit_control(request_type_out(), 0xAC, 0, 6, (unsigned char *)&mode, 4) == 4);      
    }
    virtual presel_entry get_presel_entry(size_t index) const {
      presel_entry pe;
      ASSERT_THROW(sizeof(presel_entry)==9);
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0xAB, index, 7, (unsigned char *)&pe, 9) == 9);
      return pe;
    }
    virtual void set_presel_entry(size_t index, const presel_entry& pe) {
      ASSERT_THROW(usb_control_->submit_control(request_type_out(), 0xAC, index, 7, (unsigned char *)&pe, 9) == 9);
    }

    virtual boost::uint8_t get_i2c_addr() const {
      boost::uint8_t addr;
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0x41, 0, 0, (unsigned char *)&addr, 1) == 1);
      return addr;
    }
    virtual void set_i2c_addr(boost::uint8_t addr) {
      ASSERT_THROW(usb_control_->submit_control(request_type_out(), 0x41, 0, 0, (unsigned char *)&addr, 1) == 1);
    }

    virtual boost::uint32_t get_virtual_vco_factor() const {
      boost::uint32_t factor;
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0xAB, 0, 11, (unsigned char *)&factor, 4) == 4);
      return factor;
    }
    virtual void set_virtual_vco_factor(boost::uint32_t factor) {
      ASSERT_THROW(usb_control_->submit_control(request_type_out(), 0xAB, 0, 11, (unsigned char *)&factor, 4) == 4);   
    }

    virtual si570_registers get_si570_registers() const {
      si570_registers regs;
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0xAB, 0, 10, (unsigned char *)&regs, 6) == 6);
      return regs;      
    }
    virtual si570_virtual_registers get_si570_virtual_registers() const {
      si570_virtual_registers regs;
      ASSERT_THROW(usb_control_->submit_control(request_type_in(), 0x3F, 0, 0, (unsigned char *)&regs, 6) == 6);
      return regs;      
    }
    virtual void set_si570_virtual_registers(const si570_virtual_registers& regs) {
      ASSERT_THROW(usb_control_->submit_control(request_type_out(), 0x30, 0, 0, (unsigned char *)&regs, 6) == 6);
    }

    virtual void set_abpf(boost::uint32_t index, boost::uint32_t value) {
      const ssize_t n(usb_control_->submit_control(request_type_in(), 0x17, value, index, (unsigned char *)&abpf_[0], 512));
      ASSERT_THROW(n>0 && (n%2) == 0);
      have_abpf_   = true;
      num_abpf_    = n/2 - 1;
    }
    virtual bool have_abpf() const { return have_abpf_; }
    virtual bool abpf_enabled() const { return abpf_[num_abpf_] != 0; }
    virtual size_t num_abpf() const { return num_abpf_; }
    
    // double -> counter
    template<unsigned N> static
    double c2d(boost::uint32_t c) { return double(c)/double(1<<N);  }    
    // counter -> double
    template<unsigned N> static
    boost::uint32_t d2c(double d) { return boost::uint32_t(d*(1<<N)); }

  private:
    void get_abpf() { set_abpf(255, 0); }

    boost::uint8_t request_type_out() const { 
      return LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
    }
    boost::uint8_t request_type_in() const { 
      return LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
    }
    usb_control::sptr usb_control_;
    bool have_abpf_;
    std::vector<boost::uint16_t> abpf_;
    boost::uint32_t num_abpf_;
//     bool abpf_enabled_;
  } ;

  receiver_control::sptr receiver_control::make(usb_device_handle::sptr usb_device) {
    return receiver_control::sptr(new receiver_control_impl(usb_device));
  }

  std::ostream& operator<<(std::ostream& os, const receiver_control::presel_entry& pent) {
    return os << str(boost::format("[ %7.3f - %7.3f: %2d ]")
                     % pent.freq1_MHz()
                     % pent.freq2_MHz()
                     % pent.pattern());
  }
  receiver_control::presel_entry::presel_entry(double freq1_MHz,
                                               double freq2_MHz,
                                               boost::uint8_t pattern) {
    freq1_ = receiver_control_impl::d2c<21>(freq1_MHz*4.);
    freq2_ = receiver_control_impl::d2c<21>(freq2_MHz*4.);
    pattern_ = pattern; 
  }
  double receiver_control::presel_entry::freq1_MHz() const { return receiver_control_impl::c2d<21>(freq1_/4); }
  double receiver_control::presel_entry::freq2_MHz() const { return receiver_control_impl::c2d<21>(freq2_/4); }

} // namespace FiFiSDR
