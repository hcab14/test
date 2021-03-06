// -*- C++ -*-
#ifndef CL_ARRAY_HPP_cm20170304_
#define CL_ARRAY_HPP_cm20170304_

#include "aligned_vector.hpp"
#include "cl/cl.hpp"

namespace cl {
  template <typename T>
  class array {
  public:
    typedef T value_type;
    typedef aligned_vector<value_type> vector_type;
    typedef typename vector_type::iterator iterator;
    typedef typename vector_type::const_iterator const_iterator;

    array(const cl::Context& ctx, ::size_t n)
      : _v_device(ctx, CL_MEM_READ_WRITE, n*sizeof(value_type))
      , _v_host(n, value_type(0))
      , _norm(n) {}

    array(const cl::CommandQueue& q, ::size_t n)
      : _v_device(q.getInfo<CL_QUEUE_CONTEXT>(), CL_MEM_READ_WRITE, n*sizeof(value_type))
      , _v_host(n, value_type(0))
      , _norm(n) {}

    void resize(::size_t n) {
      if (_v_host.size() == n)
	return;
      _v_host.resize(n, value_type(0));
      const auto& ctx = _v_device.getInfo<CL_MEM_CONTEXT>();
      _v_device = cl::Buffer(ctx, CL_MEM_READ_WRITE, n*sizeof(value_type));
      _norm = n;
    }

    ::size_t size() const { return _v_host.size(); }
    float norm() const { return _norm; }

    iterator   begin() { return _v_host.begin(); }
    iterator     end() { return _v_host.end(); }

    const_iterator   begin() const { return _v_host.begin(); }
    const_iterator     end() const { return _v_host.end(); }

    cl::Buffer& get_device_buffer() { return _v_device; }
    value_type& operator[](::size_t i) { return _v_host[i]; }

    const cl::Buffer& get_device_buffer() const { return _v_device; }
    const value_type& operator[](::size_t i) const { return _v_host[i]; }

    void host_to_device(cl::CommandQueue& q,
			const std::vector<cl::Event>* events_wait=NULL,
			cl::Event* event_finished=NULL) {
      const cl_bool blocking = (event_finished ? CL_TRUE : CL_FALSE);
      q.enqueueWriteBuffer(_v_device, blocking, 0, size()*sizeof(value_type), &_v_host[0], events_wait, event_finished);
    }
    void device_to_host(cl::CommandQueue& q,
			const std::vector<cl::Event>* events_wait=NULL,
			cl::Event* event_finished=NULL) {
      const cl_bool blocking = (event_finished ? CL_TRUE : CL_FALSE);
      q.enqueueReadBuffer (_v_device, blocking, 0, size()*sizeof(value_type), &_v_host[0], events_wait, event_finished);
    }

  protected:
  private:
    cl::Buffer  _v_device;
    vector_type _v_host;
    float       _norm;
  } ;
} // namespace cl

#endif // CL_ARRAY_HPP_cm20170304_
