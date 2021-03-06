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
#ifndef _FFT_ACTION_PHASE_DIFFERENTIATION_HPP_cm101106_
#define _FFT_ACTION_PHASE_DIFFERENTIATION_HPP_cm101106_

#include <complex>
#include <iostream>
#include <string>
#include <stdexcept>
#include <boost/noncopyable.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Spectrum.hpp"
#include "FFTProcessor/Filter.hpp"
#include "FFTProcessor/Result.hpp"
#include "FFTProcessor/Proxy.hpp"

namespace Action {
  template<typename T>
  class PhaseDifferentiation : public Base<T> {
  public:
    typedef Filter::WithRMS<double, frequency_vector> filter_type;
    typedef filter_type::vector_type PhaseSpectrum;
    typedef boost::posix_time::ptime ptime;

    PhaseDifferentiation(const boost::property_tree::ptree& config)
      : Base<T>("SpectrumInterval")
      , ps_(config.get<double>("fMin"),
            config.get<double>("fMax"))
      , filterDiffPhase_(Filter::LowPass<PhaseSpectrum>::make(1.0, config.get<double>("Filter.TimeConstant")),
                         Filter::LowPass<PhaseSpectrum>::make(1.0, config.get<double>("Filter.TimeConstant")),
                         1.0)
      , resultKey_(config.get<std::string>("Name"))
      , plotSpectrum_(config.get<bool>("PlotSpectrum", false)) {}

    virtual ~PhaseDifferentiation() {}

    std::string resultKey() const { return resultKey_; }
    bool plotSpectrum() const { return plotSpectrum_; }
    static double mod1(double x) {
      return ((x>1.) ? x-1. :
              (x<0.) ? x+1. : x);
    }
    virtual void perform(Proxy::Base& p, const T& s) {
      using namespace boost::posix_time;
      try {
        const double dt(  double((p.getApproxPTime() - t_).ticks())
                        / double(time_duration::ticks_per_second()));
        if (ps_.empty()) {
          ps_.fill(s, (double(*)(std::complex<double> const&))std::arg<double>);
          t_= p.getApproxPTime();
        } else {
          PhaseSpectrum psNew(ps_.fmin(), ps_.fmax(), s, (double(*)(std::complex<double> const&))std::arg<double>);
          PhaseSpectrum diffPhaseNew((psNew-ps_) / (2*M_PI)); // 2*pi = 1
          diffPhaseNew.apply(mod1);
          if (filterDiffPhase_.x().empty()) {
            filterDiffPhase_.init(p.getApproxPTime(), diffPhaseNew);
          } else {
            filterDiffPhase_.update(p.getApproxPTime(), diffPhaseNew);
          }
          ps_ = psNew;
          t_= p.getApproxPTime();
        }
        // call virtual method
        proc(p, s,
             filterDiffPhase_.x(),
             filterDiffPhase_.rms(),
             dt);
      } catch (const std::exception& e) {
        LOG_WARNING(e.what());
      } catch (...) {
        LOG_WARNING("PhaseDifferentiation::perform unknown error");
      }
    }

    // this method can be overwritten
    virtual void proc(Proxy::Base& p,
                      const T& s,
                      const PhaseSpectrum& ps,
                      const PhaseSpectrum& psrms,
                      double dt) {
      for (unsigned u=0; u<ps.size(); ++u)
        std::cout << "  --- " << dt << " " << ps[u].first << " " << ps[u].second << " " << psrms[u].second << std::endl;
    }

  protected:
  private:
    PhaseSpectrum ps_;        //
    ptime         t_;         // time when ps_ was taken
    filter_type filterDiffPhase_;
    const std::string filterType_;
    const std::string resultKey_;
    const bool plotSpectrum_;
  } ;
} // namespace Action

#endif // _FFT_ACTION_PHASE_DIFFERENTIATION_HPP_cm101106_
