// -*- C++ -*-
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <boost/format.hpp>

class PGMWriter {
public:
  typedef std::vector<std::string> Comments;
  PGMWriter(std::string fileName, 
	    size_t width,
	    const Comments& comments=Comments())
    : os_(fileName.c_str())
    , width_(width)
    , height_(0) {
    if (!os_) throw std::runtime_error("XXX");
    writeStr("P5\n");
    for (Comments::const_iterator i(comments.begin()); i!=comments.end(); ++i) 
      writeStr("# "+*i+"\n");
    writeStr(str(boost::format("%9d ")  % width_));
    heightPos_ = os_.tellp();
    writeStr(str(boost::format("%9d\n")  % height_));
    writeStr("255\n");
  }
  ~PGMWriter() {
    updateHeight();
  }

  size_t width() const { return width_; }
  size_t height() const { return height_; }

  bool writeLine(std::string s, bool performHeightUpdate=true) {
    if (!os_ || s.size() != width_) return false;
    os_.write(s.c_str(), width_);
    ++height_;
    if (performHeightUpdate)
      updateHeight();
  }

protected:
  // std::ostream& os() { return os_; }
  void updateHeight() {
    std::ostream::streampos currentPos(os_.tellp());
    os_.seekp(heightPos_);
    writeStr(str(boost::format("%9d\n")  % height_));
    os_.seekp(currentPos);
  }

  bool writeStr(std::string s) {
    os_.write(s.c_str(), s.size());
    return bool(os_);
  }
private:
  std::ofstream os_;
  const size_t width_;
  size_t height_;
  std::ostream::streampos heightPos_;
} ;

int main()
{
  PGMWriter::Comments c;
  c.push_back("Hello");
  c.push_back("World!");
  PGMWriter p("test.pgm", 100, c);
  std::string line;
  for (unsigned u(0); u<100; ++u)
    line.push_back(u);
  for (unsigned u(0); u<100; ++u)
    p.writeLine(line);
}