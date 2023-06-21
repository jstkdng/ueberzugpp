require "pty"

class Ueberzugpp < Formula
  desc "Drop in replacement for ueberzug written in C++"
  homepage "https://github.com/jstkdng/ueberzugpp"
  url "https://github.com/jstkdng/ueberzugpp/archive/refs/tags/v2.8.7.tar.gz"
  sha256 "650fa80f632f4ad61313cec10d392ea092b8b2b74dd5ba5c5339aa01f6b8543a"
  license "GPL-3.0-or-later"

  depends_on "cli11" => :build
  depends_on "cmake" => :build
  depends_on "nlohmann-json" => :build
  depends_on "pkg-config" => :build
  depends_on "cpp-gsl" => :build
  depends_on "fmt"
  depends_on "libsixel"
  depends_on "openssl@1.1"
  depends_on "spdlog"
  depends_on "tbb"
  depends_on "vips"
  depends_on "chafa"

  def install
    system "cmake", "-S", ".", "-B", "build",
                    "-DENABLE_X11=OFF",
                    "-DENABLE_OPENCV=OFF",
                    "-DENABLE_WAYLAND=OFF"
                    *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    ENV["TMPDIR"] = testpath
    ENV["SPDLOG_LEVEL"] = "debug"
    __, secondary = PTY.open
    read, __ = IO.pipe
    pid = spawn("#{bin}/ueberzugpp layer -o iterm2", in: read, out: secondary)
    sleep(0.1)
    read.close
    secondary.close
    Process.kill("TERM", pid)

    assert_predicate testpath/"ueberzugpp-#{ENV["USER"]}.log", :exist?
  end
end
