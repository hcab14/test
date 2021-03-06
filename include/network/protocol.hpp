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
#ifndef _PROTOCOL_HPP_cm100625_
#define _PROTOCOL_HPP_cm100625_

#include <boost/cstdint.hpp>
#include <boost/array.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "logging.hpp"

namespace network {
  namespace protocol {
    // -----------------------------------------------------------------------------
    // generic header for any type of data
    //
    class __attribute__((__packed__)) header {
    public:
      typedef boost::posix_time::ptime ptime;
      typedef boost::posix_time::time_duration time_duration;

      header(std::string     id="01234567",
             ptime           approx_ptime=boost::posix_time::not_a_date_time,
             boost::uint32_t stream_number=0,
             boost::uint32_t length=0)
        : stream_number_(stream_number)
        , length_(length) {
        std::copy(reinterpret_cast<char*>(&approx_ptime),
                  reinterpret_cast<char*>(&approx_ptime)+sizeof(ptime),
                  approx_ptime_);
        ASSERT_THROW(id.size() == 8);
        std::copy(id.begin(), id.end(), id_);
      }

      std::string     id()            const { return std::string(id_, id_+8); }
      ptime           approx_ptime()  const {
        ptime result;
        std::copy(approx_ptime_, approx_ptime_+sizeof(ptime), reinterpret_cast<char*>(&result));
        return result; 
      }
      boost::uint32_t stream_number() const { return stream_number_; }
      boost::uint32_t length()        const { return length_; }

      void set_stream_number(boost::uint32_t stream_number) {
        stream_number_ = stream_number;
      }

      const char* begin() const { return reinterpret_cast<const char*>(this); }
      const char* end() const { return reinterpret_cast<const char*>(this) + sizeof(header); }

      friend std::ostream& operator<<(std::ostream& os, const header& h) {
        return os << "id='" << h.id()
                  << "' "<< h.approx_ptime()
                  << " stream_number=" << h.stream_number()
                  << " len=" << h.length();
      }

    protected:
    private:
      char                 id_[8];         // 8-byte identifier         |  8b
      boost::uint8_t       approx_ptime_[sizeof(ptime)];  // approximate time tag  | 8b (12b)
      boost::uint32_t      stream_number_; // stream number             |  4b
      boost::uint32_t      length_;        // length of following data  |  4b
      //                                                          total:  24b (28b)
    } ;

    // -----------------------------------------------------------------------------
    // header for sampled I/Q data
    //
    class __attribute__((__packed__)) iq_info {
    public:
      iq_info(boost::uint32_t sample_rate_Hz=1,
              double          center_frequency_Hz=0.,
              char            sample_type='I',
              boost::uint8_t  bytes_per_sample=3,
              float           offset_ppb=0.,
              float           offset_ppb_rms=0.)
        :
#if COMP_OLD
        sample_rate_Hz_(sample_rate_Hz)
        , center_frequency_Hz_(center_frequency_Hz)
        , sample_type_(sample_type)
        , bytes_per_sample_(bytes_per_sample)
        , offset_ppb_(offset_ppb)
        , offset_ppb_rms_(offset_ppb_rms)
#else
        center_frequency_Hz_(center_frequency_Hz) 
        , offset_ppb_(offset_ppb)
        , offset_ppb_rms_(offset_ppb_rms)
        , sample_rate_Hz_(sample_rate_Hz)
        , sample_type_(sample_type)
        , bytes_per_sample_(bytes_per_sample)
#endif
      {
        dummy_[0] = dummy_[1] = 0;
      }

      boost::uint32_t sample_rate_Hz()      const { return sample_rate_Hz_; }
      double          center_frequency_Hz() const { return center_frequency_Hz_; }
      char            sample_type()         const { return sample_type_; }
      boost::uint8_t  bytes_per_sample()    const { return bytes_per_sample_; }
      float           offset_ppb()          const { return offset_ppb_; }
      float           offset_ppb_rms()      const { return offset_ppb_rms_; }

      const char* begin() const { return reinterpret_cast<const char*>(this); }
      const char* end()   const { return begin() + sizeof(iq_info); }

      friend std::ostream& operator<<(std::ostream& os, const iq_info& h) {
        return os << "sample_rate_Hz="      << h.sample_rate_Hz()
                  << " center_frequency_Hz=" << h.center_frequency_Hz()
                  << " sample_type='"       << h.sample_type() << "'"
                  << " bytes_per_sample="   << int(h.bytes_per_sample())
                  << " offset_ppb="         << h.offset_ppb()
                  << " offset_ppb_rms="     << h.offset_ppb_rms();
      }
    protected:
    private:
#if COMP_OLD
      boost::uint32_t sample_rate_Hz_;       //  sample rate [Hz]        |  4b
      double          center_frequency_Hz_;  //  center frequency [Hz]   |  8b
      char            sample_type_;          //                          |  1b
      boost::uint8_t  bytes_per_sample_;     //                          |  1b
      float           offset_ppb_;           //  deviation from nominal  |  4b
      float           offset_ppb_rms_;       //  deviation from nominal  |  4b
      boost::int8_t   dummy_[2];             //  for future use          |  2b
#else
      double          center_frequency_Hz_;  //  center frequency [Hz]   |  8b
      float           offset_ppb_;           //  deviation from nominal  |  4b
      float           offset_ppb_rms_;       //  deviation from nominal  |  4b
      boost::uint32_t sample_rate_Hz_;       //  sample rate [Hz]        |  4b
      char            sample_type_;          //                          |  1b
      boost::uint8_t  bytes_per_sample_;     //                          |  1b
      boost::int8_t   dummy_[2];             //  for future use          |  2b
#endif
      //                                                            total: 24b
    } ;

    class __attribute__((__packed__)) directory_entry {
    public:
      directory_entry(boost::uint32_t stream_number,
                      boost::uint32_t length_of_name)
        : stream_number_(stream_number)
        , length_of_name_(length_of_name) {}

      boost::uint32_t stream_number() const { return stream_number_; }
      boost::uint32_t length_of_name() const { return length_of_name_; }

      static boost::uint32_t name_offset() { return 2*sizeof(boost::uint32_t); }
      boost::uint32_t size() const { return name_offset() + length_of_name(); }
      const char* begin() const { return reinterpret_cast<const char*>(this); }

      static std::string serialize(boost::uint32_t stream_number, std::string name) {
        std::string result;
        const directory_entry de(stream_number, name.size());
        std::copy(de.begin(), de.begin()+2*sizeof(boost::uint32_t), std::back_inserter(result));
        result += name;
        return result;
      }

    protected:
    private:
      directory_entry(const directory_entry& );
      directory_entry& operator=(const directory_entry& );

      boost::uint32_t stream_number_;   // stream number
      boost::uint32_t length_of_name_;  // number of following characters
    } ;

  } // namespace protocol
} // namespace network

#endif // _PROTOCOL_HPP_cm100625_
