// -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil  -*-
// $Id$
#ifndef _FFT_RESULT_SPECTRUM_PEAK_HPP_cm101026_
#define _FFT_RESULT_SPECTRUM_PEAK_HPP_cm101026_

#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>

#include "FFTResultCalibration.hpp"
#include "Spectrum.hpp"

namespace Result {
  class SpectrumPeak : public Base {
  public:
    typedef boost::shared_ptr<SpectrumPeak> Handle;
    typedef frequency_vector<double> PowerSpectrum;
    SpectrumPeak(double fReference)
      : Base("SpectrumPeak")
      , fReference_(fReference) 
      , fMeasured_(0.) 
      , fMeasuredRMS_(1.) 
      , strength_(0.) 
      , strengthRMS_(1.)
      , ratio_(1.) {}

    bool findPeak(const SpectrumBase& s, const PowerSpectrum& ps, double minRatio) {
      return ps.empty() ? false : findPeak(s, ps, ps.front().first, ps.back().first, minRatio);
    }
    // find a peak in the range [fMin, fMax] (including fMax)
    bool findPeak(const SpectrumBase& s, const PowerSpectrum& ps,
                  double fMin, double fMax, double minRatio) {
      const size_t i0(ps.freq2index(fMin));
      const size_t i1(ps.freq2index(fMax));
      // PowerSpectrum::const_iterator
      //   iMin(std::min_element(ps.begin()+i0, ps.begin()+i1+1,
      //                         PowerSpectrum::cmpSecond));
      PowerSpectrum::const_iterator
        iMax(std::max_element(ps.begin()+i0, ps.begin()+i1+1,
                              PowerSpectrum::cmpSecond));
      
      const size_t indexMax(std::distance(ps.begin(), iMax));

      // estimate frequency 
      const std::pair<double, double> 
        peakFrequency(estimatePeakFrequency(ps, indexMax));

      // estimate background
      const std::pair<double, double> 
        background(estimateBackground(ps, indexMax, i0, i1));
      
      // for (int i=-2; i<=2; ++i) {
      //   std::cout << "FP_ " << ps[indexMax+i].first << " " << ps[indexMax+i].second << " "
      //             << std::arg(s[s.freq2index(ps[indexMax+i].first)])/(2*M_PI) 
      //   	  << std::endl;
      // }

      ratio_ = (background.first != 0.0) ? iMax->second / background.first : 1.0;
      if (ratio_ < minRatio)
        LOG_WARNING(str(boost::format("ratio < minRatio : %f %f %f") 
                        % ratio_
                        % background.first
                        % background.second));

      // TODO: error propagation
      fMeasured_    = peakFrequency.first;
      fMeasuredRMS_ = peakFrequency.second;
      strength_     = (ratio_ > minRatio) ? iMax->second     : background.first;
      strengthRMS_  = (ratio_ > minRatio) ? iMax->second/10. : background.second; // TODO
      return (ratio_ > minRatio);
    }

    std::pair<double, double> 
    estimatePeakFrequency(const PowerSpectrum& ps, size_t indexMax) const {
      double sum(0.0);
      double weight(0.0);
      for (size_t u(indexMax-std::min(indexMax, size_t(3))); 
           u<(indexMax+4) && u < ps.size(); ++u) {
        sum    += ps[u].second * ps[u].first;
        weight += ps[u].second;
      }
      return (weight != 0.0) ? cal(sum/weight) : std::make_pair(0.0, 1.0);
    }

    std::pair<double, double> 
    estimateBackground(const PowerSpectrum& ps, size_t indexMax,
                       size_t i0, size_t i1) const {
      double sum(0.0), sum2(0.0);
      size_t counter(0);
      for (size_t u(i0); u<i1+1; ++u) {
        if (std::abs(int(u) - int(indexMax)) < 4) continue;
        sum += ps[u].second;
        sum2 += ps[u].second * ps[u].second;
        ++counter;
      }      
      return (counter != 0) 
        ? std::make_pair(sum/counter, 
                         std::sqrt(sum2/counter - sum*sum/(counter*counter)))
        : std::make_pair(0., 1.);
    }

    virtual ~SpectrumPeak() {}
    virtual std::string toString() const { 
      std::stringstream ss; 
      ss << Base::toString() 
         << " fReference="   << fReference()
         << " fMeasured="    << fMeasured()
         << " fMeasuredRMS=" << fMeasuredRMS()
         << " strength="     << strength()
         << " strengthRMS="  << strengthRMS()
         << " ratio="        << ratio();
      return ss.str();
    }
    double fReference() const { return fReference_; }
    double fMeasured() const { return fMeasured_; }
    double fMeasuredRMS() const { return fMeasuredRMS_; }
    double strength() const { return strength_; }
    double strengthRMS() const { return strengthRMS_; }
    double ratio() const { return ratio_; }

    // this method is overwritten e.g. in CalibratedSpectrumPeak
    virtual std::pair<double, double> cal(double f) const {
      return std::make_pair(f, double(1));
    }

    virtual boost::filesystem::fstream& dumpHeader(boost::filesystem::fstream& os,
                                                   boost::posix_time::ptime t) const {      
      os << "# Frequency = " << boost::format("%12.3f") % fReference() << " [Hz]\n";
      Base::dumpHeader(os, t) 
        << "fMeasured_Hz fMeasuredRMS_Hz strength_dBm strengthRMS_dBm S/N_dB ";
      return os;
    }
    virtual boost::filesystem::fstream& dumpData(boost::filesystem::fstream& os,
                                                 boost::posix_time::ptime t) const {
      Base::dumpData(os, t)
        << boost::format("%12.3f") % fMeasured() << " "
        << boost::format("%6.3f")  % fMeasuredRMS() << " "
        << boost::format("%7.2f")  % (20.*std::log10(strength())) << " "
        << boost::format("%7.2f")  % (20.*std::log10(strengthRMS())) << " "
        << boost::format("%7.2f")  % (20.*std::log10(ratio())) << " ";
      return os;
    }

  private:
    double fReference_;
    double fMeasured_;
    double fMeasuredRMS_;
    double strength_;
    double strengthRMS_;
    double ratio_;
  } ;
} // namespace Result

#endif // _FFT_RESULT_SPECTRUM_PEAK_HPP_cm101026_
