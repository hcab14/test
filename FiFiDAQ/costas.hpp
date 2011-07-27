// -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil  -*-
// $Id$
#ifndef _COSTAS_HPP_cm110707_
#define _COSTAS_HPP_cm110707_

#include <vector>
#include <complex>

namespace filter {
  // costas phase-locked loop
  template <typename T,
            class LOOP_FILTER,
            class INTEGRATOR_FREQ,
            class INTEGRATOR_PHASE>
  class costas {
  public:
    typedef T float_type;
    typedef typename std::complex<float_type> complex_type;
    typedef typename std::vector<complex_type> complex_vector_type;
    
    costas(double fc,
           double fs,
           double alpha,
           double beta,
           const LOOP_FILTER& loop_filter,
           const INTEGRATOR_FREQ& integrator_freq,
           const INTEGRATOR_PHASE& integrator_phase)
      : loop_filter_(loop_filter)
      , integrator_freq_(integrator_freq)
      , integrator_phase_(integrator_phase)
      , fc_(fc)
      , fs_(fs)
      , alpha_(alpha)
      , beta_(beta) {
      integrator_freq_.reset(fc);
    }
    
    void reset() {
      loop_filter_.reset();
      integrator_freq_.reset(fc_);
      integrator_phase_.reset();
    }
    
    double process(complex_type s) {
      const complex_type c(s * std::exp(-complex_type(0, theta())));
      const float_type err(loop_filter_.process(c.real() * c.imag()));
      std::cerr << "err= " << err << std::endl;
      return integrator_phase_.process(alpha_*err + integrator_freq_.process(beta_*err)/fs_*2*M_PI);
    }
    
    float_type theta() const { return integrator_phase_.get(); }
    float_type f1() const { return integrator_freq_.get(); }

    double alpha() const { return alpha_; }
    double beta() const { return beta_; }
    
    const LOOP_FILTER& loop_filter() const { return loop_filter_; }
    const INTEGRATOR_FREQ& integrator_freq() const { return integrator_freq_; }
    const INTEGRATOR_PHASE& integrator_phase() const { return integrator_phase_; }
  protected:
  private:
    LOOP_FILTER loop_filter_;
    INTEGRATOR_FREQ  integrator_freq_;
    INTEGRATOR_PHASE integrator_phase_;
    double fc_;
    double fs_;
    double alpha_;
    double beta_;
  } ;
} // namespace filter
#endif // _COSTAS_HPP_cm110707_
