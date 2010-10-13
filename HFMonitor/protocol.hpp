// -*- C++ -*-
// $Id$
#ifndef _PROTOCOL_HPP_250610_
#define _PROTOCOL_HPP_250610_

#include <boost/cstdint.hpp>

class Header {
public:
  Header(boost::int64_t  sampleNumber=0,
	 boost::uint32_t sampleRate=1,
	 boost::uint32_t ddcCenterFrequency=0,
	 boost::uint32_t numberOfSamples=0,
	 boost::uint8_t  samplingRateIndex=0,
	 boost::uint8_t  attenId=0,
	 boost::uint8_t  adcPresel=1,
	 boost::uint8_t  adcPreamp=0,
	 boost::uint8_t  adcDither=0)
    : sampleNumber_      (sampleNumber)
    , sampleRate_        (sampleRate)
    , ddcCenterFrequency_(ddcCenterFrequency)
    , numberOfSamples_   (numberOfSamples)
    , samplingRateIndex_ (samplingRateIndex)
    , attenId_           (attenId)
    , adcPresel_         (adcPresel)
    , adcPreamp_         (adcPreamp)
    , adcDither_         (adcDither) {}

  boost::int64_t  sampleNumber()       const { return sampleNumber_; }
  boost::int64_t& sampleNumber()             { return sampleNumber_; }
  boost::uint32_t sampleRate()         const { return sampleRate_; }
  boost::uint32_t ddcCenterFrequency() const { return ddcCenterFrequency_; }
  boost::uint32_t numberOfSamples()    const { return numberOfSamples_; }
  boost::uint8_t  samplingRateIndex()  const { return samplingRateIndex_; }
  boost::uint8_t  attenId()            const { return attenId_; }
  boost::uint8_t  adcPresel()          const { return adcPresel_; }
  boost::uint8_t  adcPreamp()          const { return adcPreamp_; }
  boost::uint8_t  adcDither()          const { return adcDither_; }

  void setNumberOfSamples(boost::uint32_t n)      { numberOfSamples_ = n; }

  friend std::ostream& operator<<(std::ostream& os, const Header& h) {
    return os << "sampleNumber="        << h.sampleNumber() 
	      << " sampleRate="         << h.sampleRate()
	      << " ddcCenterFrequency=" << h.ddcCenterFrequency()
	      << " numberOfSamples="    << h.numberOfSamples()
	      << " samplingRateIndex="  << int(h.samplingRateIndex())
	      << " attenId="            << int(h.attenId())
	      << " adcPreamp="          << int(h.adcPreamp())
	      << " adcDither="          << int(h.adcDither());
  }
  
  bool hasEqualParameters(const Header& h) const {
    return ( sampleRate_         == h.sampleRate_ &&
	     ddcCenterFrequency_ == h.ddcCenterFrequency_ &&
	     samplingRateIndex_  == h.samplingRateIndex_ &&
	     attenId_            == h.attenId_ &&
	     adcPresel_          == h.adcPresel_ &&
	     adcPreamp_          == h.adcPreamp_ &&
	     adcDither_          == h.adcDither_ );
  }

protected:
private:
  boost::int64_t  sampleNumber_;
  boost::uint32_t sampleRate_;
  boost::uint32_t ddcCenterFrequency_;
  boost::uint32_t numberOfSamples_;
  boost::uint8_t  samplingRateIndex_ : 3;
  boost::uint8_t  attenId_           : 2;
  boost::uint8_t  adcPresel_         : 1;
  boost::uint8_t  adcPreamp_         : 1;
  boost::uint8_t  adcDither_         : 1;
} ;

#endif // _PROTOCOL_HPP_250610_