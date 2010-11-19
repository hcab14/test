// -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil  -*-
// $Id$
#include <iostream>
#include <iterator>
#include <vector>
#include <set>
#include <string>
#include <sstream>
#include <deque>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>

#include "perseus++.h"
#include "Filter.hpp"
#include "protocol.hpp"
#include "run.hpp"

class processor_status : private boost::noncopyable {
public:
  typedef boost::posix_time::ptime ptime;
  processor_status() {}
  ~processor_status() {}
  
protected:

private:
  
} ;

class data_connection : private boost::noncopyable {
public:
  typedef boost::posix_time::ptime ptime;
  typedef boost::posix_time::time_duration time_duration;

  typedef boost::shared_ptr<data_connection> sptr;
  typedef boost::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket_ptr;
  typedef std::vector<char> Data;
  typedef boost::shared_ptr<Data> data_ptr;
  typedef std::pair<ptime, data_ptr> ptime_data_pair;
  typedef std::deque<ptime_data_pair> ListOfPackets;

  data_connection(boost::asio::io_service& io_service,
                  boost::asio::strand& strand,
                  tcp_socket_ptr p,
                  size_t max_total_size=40*1024*1024,
                  time_duration max_queue_delay=boost::posix_time::minutes(5))
    : io_service_(io_service)
    , strand_(strand)
    , tcp_socket_ptr_(p)
    , isOpen_(true)
    , max_total_size_(max_total_size)
    , max_queue_delay_(max_queue_delay)
    , max_delay_(boost::posix_time::seconds(0)) {}

  ~data_connection() {
    close();
  }

  void close() {  tcp_socket_ptr_->close(); isOpen_= false; }
  bool is_open() const { return isOpen_; }

  void pop_front() { listOfPackets_.pop_front(); }
  void pop_back()  { listOfPackets_.pop_back(); }

  ListOfPackets::const_reference front() const { return listOfPackets_.front(); }
  ListOfPackets::const_reference back() const  { return listOfPackets_.back(); }

  bool   empty() const { return listOfPackets_.empty(); }
  size_t size()  const { return listOfPackets_.size(); }

  size_t total_size() const {
    size_t sum(0);
    BOOST_FOREACH(const ListOfPackets::value_type& lp, listOfPackets_)
      sum += lp.second->size();
    return sum;
  }

  boost::asio::ip::tcp::endpoint local_endpoint(boost::system::error_code& ec) const {
    return tcp_socket_ptr_->local_endpoint(ec);
  }
  boost::asio::ip::tcp::endpoint remote_endpoint(boost::system::error_code& ec) const {
    return tcp_socket_ptr_->remote_endpoint(ec);
  }

  time_duration delay(ptime t) const {
    return (empty()) ? boost::posix_time::seconds(0) : t - front().first;
  }

  time_duration max_delay() const { return max_delay_; }
  void reset_max_delay() { max_delay_ = boost::posix_time::seconds(0); }

  bool push_back(ptime t, const data_ptr& dp) {
    max_delay_ = std::max(max_delay_, delay(t));
    size_t n_omit(0);
    for (; not empty() && (total_size() > max_total_size_ || front().first+max_queue_delay_ < t); ++n_omit)
      pop_front();
    if (n_omit != 0)
      std::cout << "omitted #" << n_omit << " data packets" << std::endl;
    if (is_open()) {
      const bool listOfPacketsWasEmpty(empty());
      listOfPackets_.push_back(std::make_pair(t, dp));
      if (listOfPacketsWasEmpty)
        async_write();
      return true;
    }
    return false;
  }

  void async_write() {
    if (!empty()) {
      data_ptr dataPtr(front().second);
      boost::asio::async_write(*tcp_socket_ptr_,
                               boost::asio::buffer(&dataPtr->front(), dataPtr->size()),
                               strand_.wrap(boost::bind(&data_connection::handle_write,
                                                        this,
                                                        boost::asio::placeholders::error,
                                                        boost::asio::placeholders::bytes_transferred)));
    }
  }

  void handle_write(const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (ec) {
      close();
    } else {
      pop_front();
      async_write();
    }
  }
protected:
private:
  boost::asio::io_service& io_service_;
  boost::asio::strand&     strand_;
  tcp_socket_ptr           tcp_socket_ptr_;
  bool                     isOpen_;
  const size_t             max_total_size_;
  const time_duration      max_queue_delay_;
  time_duration            max_delay_;
  ListOfPackets            listOfPackets_;
} ;

class server : private boost::noncopyable {
public:
  typedef boost::shared_ptr<server> sptr;
  typedef boost::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket_ptr;
  typedef std::set<tcp_socket_ptr> Sockets;
  typedef boost::shared_ptr<data_connection> data_connection_ptr;
  typedef std::set<data_connection_ptr> data_connections;
  typedef boost::posix_time::ptime ptime;
  typedef boost::posix_time::time_duration time_duration;

  server(boost::asio::io_service& io_service,
         const boost::asio::ip::tcp::endpoint& endpoint_ctrl,
         const boost::asio::ip::tcp::endpoint& endpoint_data,
         Perseus::ReceiverPtr recPtr,
         const boost::property_tree::ptree& config)
    : io_service_(io_service)
    , strand_(io_service)
    , acceptor_ctrl_(io_service, endpoint_ctrl)
    , acceptor_data_(io_service, endpoint_data)
    , recPtr_(recPtr)
    , usbBufferSize_(config.get<unsigned>("perseus.USBBufferSize"))
    , sampleNumber_(0)
    , dtCallback_(0,0,0,
                  usbBufferSize_/6*time_duration::ticks_per_second()/recPtr_->sampleRate()) {
    // control setup
    {
      acceptor_ctrl_.set_option(boost::asio::socket_base::reuse_address(true));
      acceptor_ctrl_.listen();
      tcp_socket_ptr new_socket(new boost::asio::ip::tcp::socket(acceptor_ctrl_.get_io_service()));
      acceptor_ctrl_.async_accept(*new_socket,
                                  strand_.wrap(boost::bind(&server::handle_accept_ctrl, this,
                                                           boost::asio::placeholders::error, new_socket)));
    }
    // data setup
    {
      acceptor_data_.set_option(boost::asio::socket_base::reuse_address(true));
      acceptor_data_.listen();
      tcp_socket_ptr new_socket(new boost::asio::ip::tcp::socket(acceptor_data_.get_io_service()));
      acceptor_data_.async_accept(*new_socket,
                                  strand_.wrap(boost::bind(&server::handle_accept_data, this,
                                                           boost::asio::placeholders::error, new_socket)));
    }
    // PTimeLowpassFilters:
    {
      using boost::property_tree::ptree;
      std::cout << "Cascaded Lowpass Filters: " << std::endl;
      const double dtCallbackSec(double(dtCallback_.ticks())/time_duration::ticks_per_second());
      BOOST_FOREACH(const ptree::value_type& filter, config.get_child("perseus.CascadedPTimeLowPass")) {
        if (filter.first != "Tau_Sec") {
          std::cout << "  + unknown key: [" << filter.first << "]" << std::endl;
          continue;
        }
        const double filterTimeconstant(boost::lexical_cast<double>(filter.second.data()));
        std::cout << "  + Lowpass Filter " << filter.first << "=" << filterTimeconstant << std::endl;
        ptimeFilter_.add(Filter::PTimeLowPass::make(dtCallbackSec, filterTimeconstant));
        dtFilter_.add(Filter::LowPass<double>::make(dtCallbackSec, filterTimeconstant));        
      }
      const ptime now(boost::posix_time::microsec_clock::universal_time());
      ptimeFilter_.init(now, now);
      dtFilter_.init(now, dtCallbackSec);
      ptimeOfCallback_ = ptimeDataMeasure_ = now;
    }
    recPtr_->startAsyncInput(usbBufferSize_, server::receiverCallback, this);
    std::cout << "ptime_ = " << ptimeOfCallback_ << " " 
              << "dt= " << 1e-6*dtCallback_.total_microseconds() << std::endl;
  }
  
  void handle_accept_ctrl(const boost::system::error_code& ec, tcp_socket_ptr socket) {
    std::cout << "servce::handle_accept_ctrl error_code= " << ec << std::endl;
    std::cout << "remote endpoint= " << socket->remote_endpoint() << std::endl;
    if (!ec) {
      ctrl_sockets_.insert(socket);
      std::cout << "socket remote_ep = " << socket->remote_endpoint() << " "
                << (socket->is_open() ? "open" : "closed") << std::endl;
      
      tcp_socket_ptr new_socket(new boost::asio::ip::tcp::socket(acceptor_ctrl_.get_io_service()));      
      acceptor_ctrl_.async_accept(*new_socket,
                                  strand_.wrap(boost::bind(&server::handle_accept_ctrl, this,
                                                           boost::asio::placeholders::error, new_socket)));
    }
  }
  void handle_accept_data(const boost::system::error_code& ec, tcp_socket_ptr socket) {
    std::cout << "servce::handle_accept_data error_code= " << ec << std::endl;
    if (!ec) {
      std::cout << "remote endpoint= " << socket->remote_endpoint() << std::endl;
      data_connections_.insert(data_connection_ptr(new data_connection(io_service_, strand_, socket)));
      tcp_socket_ptr new_socket(new boost::asio::ip::tcp::socket(acceptor_data_.get_io_service()));
      acceptor_data_.async_accept(*new_socket,
                                  strand_.wrap(boost::bind(&server::handle_accept_data, this,
                                                           boost::asio::placeholders::error, new_socket)));
    } else {
      // error 
    }
  }

  Header getHeader(const unsigned nSamples, ptime approxPTime) {
    return Header((sampleNumber_+=nSamples) - nSamples,
                  recPtr_->sampleRate(),
                  recPtr_->ddcCenterFrequency(),
                  nSamples,
                  0, // TODO
                  recPtr_->attenId(),
                  recPtr_->enablePresel(),
                  recPtr_->enablePreamp(),
                  recPtr_->enableDither(),
                  approxPTime);
  }

  static int receiverCallback(void *buf, int buf_size, void *extra) {
    server* sp= (server* )extra;
    const unsigned nSamples = buf_size/6;

    const ptime now(boost::posix_time::microsec_clock::universal_time());
    const time_duration dt(now - sp->ptimeOfCallback_);

    const ptime oldFilterTime(sp->ptimeFilter_.x());
    const bool doInterpolation(std::abs(dt.ticks() - sp->dtCallback_.ticks()) > sp->dtCallback_.ticks()/10); 
    const ptime nowInterpolated(doInterpolation ? oldFilterTime + sp->dtCallback_ : now);
    sp->ptimeFilter_.update(nowInterpolated, nowInterpolated);
    sp->dtFilter_.update(nowInterpolated, (doInterpolation 
                                           ? double(sp->dtCallback_.ticks())
                                           : double(dt.ticks())) / time_duration::ticks_per_second());
    sp->ptimeOfCallback_= now;
#if 1
    std::cout << "receiverCallback "
              << sp->ptimeOfCallback_ << " "
              << sp->ptimeFilter_.x() << " "
              << dt << " "
              << sp->dtCallback_ << " "
              << (sp->ptimeFilter_.x()-oldFilterTime).ticks() << " " << sp->dtCallback_.ticks() << " "
              << sp->dtFilter_.x() << " " 
              << sp->dtFilter_.x() / double(sp->dtCallback_.ticks())*time_duration::ticks_per_second()-1. << " "
              << sp->ptimeOfCallback_ - sp->ptimeFilter_.x() << " "
              << ((std::abs(dt.ticks() - sp->dtCallback_.ticks()) < sp->dtCallback_.ticks()/10) ? "OK" : "IP") 
              <<  std::endl;
#endif
    // keep only one copy of the data in memory even when there are several clients
    const Header header(sp->getHeader(nSamples, oldFilterTime));
    data_connection::data_ptr dp(new std::vector<char>);
    std::copy((char*)&header, (char*)&header+sizeof(Header), std::back_inserter(*dp));
    std::copy((char*)buf,     (char*)buf+buf_size,           std::back_inserter(*dp));

    sp->io_service_.dispatch(boost::bind(&server::sendDataToClients, sp, sp->ptimeFilter_.x(), dp));

    boost::system::error_code ec;
    const size_t max_poll_actions(100*1000);
    for (size_t u(0); sp->io_service_.poll(ec) > 0 && u < max_poll_actions; ++u)
      if (ec) break;
    return 0;
  }

  void sendDataToClients(const ptime& t, const data_connection::data_ptr& dp) {
    bc_status();
    for (data_connections::iterator i(data_connections_.begin()); i!=data_connections_.end();) {
      if ((*i)->push_back(t, dp))
        ++i;
      else 
        data_connections_.erase(i++);
    }
  }

  void bc_status() {
    const unsigned buffers_per_second(0.5 + double(recPtr_->sampleRate()) / double(usbBufferSize_));
    const unsigned moduloSize(0.5 + buffers_per_second * usbBufferSize_);
    // every 1 second status -> log
    if (sampleNumber_ % moduloSize == 0) {
      // round dt modulo [time of 850 samples]
      const int dt_usec (0.5 + 1e6*(  double((ptimeOfCallback_ - ptimeDataMeasure_).ticks())
                                   / double(time_duration::ticks_per_second())));
      const int dt0_usec(0.5 + 1e6* usbBufferSize_/6. / recPtr_->sampleRate());
      const double rate(1e6* double(moduloSize) / double(dt0_usec * int(0.5+double(dt_usec)/double(dt0_usec))));
      std::stringstream logMessage;
      logMessage << "data_connections_.size*() = " << data_connections_.size() 
                 << " sampleNumber=" << sampleNumber_ << " "
                 << " dataRate= " << rate;
      log(logMessage.str());
      ptimeDataMeasure_ = ptimeOfCallback_;
      BOOST_FOREACH(const data_connections::value_type& dc, data_connections_) {
        std::stringstream logMessage;
        boost::system::error_code ecl, ecr;
        logMessage << dc->local_endpoint(ecl) << " - "
                   << dc->remote_endpoint(ecr)
                   << " : delay[ms] = "
                   << 1000*dc->max_delay().ticks() / time_duration::ticks_per_second();
        dc->reset_max_delay();
        log(logMessage.str());
      }
    }
  }

  void log(std::string message) {
    // TBD
    std::cout << message << std::endl;
  }

  void stop() {
    recPtr_->stopAsyncInput();
    io_service_.post(boost::bind(&server::do_stop, this));
    // process the remaining actions
    io_service_.run();
  }

  void do_stop() {
    ctrl_sockets_.clear();
    data_connections_.clear();
    io_service_.stop();
  }

protected:
private:
  boost::asio::io_service&       io_service_;
  boost::asio::strand            strand_;
  boost::asio::ip::tcp::acceptor acceptor_ctrl_;
  boost::asio::ip::tcp::acceptor acceptor_data_;
  Sockets                        ctrl_sockets_;
  Perseus::ReceiverPtr           recPtr_;
  unsigned                       usbBufferSize_;
  boost::int64_t                 sampleNumber_;
  time_duration                  dtCallback_;
  ptime                          ptimeOfCallback_;
  ptime                          ptimeDataMeasure_;
  Filter::Cascaded<ptime>        ptimeFilter_;
  Filter::Cascaded<double>       dtFilter_;
  data_connections               data_connections_;

  boost::asio::streambuf request_;
  boost::asio::streambuf response_;
} ;


int main(int argc, char* argv[])
{
  try {
    std::string filename((argc > 1 ) ? argv[1] : "config.xml");
    boost::property_tree::ptree pt;
    read_xml(filename, pt);
    std::cout << "debug.filename=" << pt.get<std::string>("debug.filename") << std::endl;

    perseus_set_debug(3);

    Perseus p;
    const unsigned numPerseus(p.numPerseus());
    if (numPerseus == 0)
      throw std::runtime_error("numPerseus == 0");

    Perseus::ReceiverPtr pp(p.getReceiverPtr(0));

    pp->downloadFirmware();
    pp->fpgaConfig(pt.get<int>("perseus.fpga.samplerate"));                

    // todo: get from config
    pp->setAttenuator(PERSEUS_ATT_0DB);

    // todo: get flags from config
    pp->setADC(true, false);

    // todo: get flag from config
    pp->setDdcCenterFreq(pt.get<double>("perseus.qrg_Hz"), 1);

    boost::asio::io_service io_service;
    server::sptr sp;
    {
      wait_for_signal w;
      w.add_signal(SIGINT)
       .add_signal(SIGQUIT)
       .add_signal(SIGTERM);
      sp = server::sptr(new server(io_service,
                                   boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 
                                                                  pt.get<unsigned>("server.ctrl.port")),
                                   boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 
                                                                  pt.get<unsigned>("server.data.port")),
                                   pp,
                                   pt));
    }
    sp->stop();
  } catch (std::exception &e) {
    std::cout << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
