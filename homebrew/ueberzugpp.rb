require "pty"

class Ueberzugpp < Formula
  desc "Drop in replacement for ueberzug written in C++"
  homepage "https://github.com/jstkdng/ueberzugpp"
  url "https://github.com/jstkdng/ueberzugpp/archive/refs/tags/v2.8.2.tar.gz"
  sha256 "202eee24b706f9120cb506de7a6cf8f46d1ea3bdf2fbca5e53513b21c5be9695"
  license "GPL-3.0-or-later"

  depends_on "cli11" => :build
  depends_on "cmake" => :build
  depends_on "cppzmq" => :build
  depends_on "nlohmann-json" => :build
  depends_on "pkg-config" => :build
  depends_on "cpp-gsl" => :build
  depends_on "fmt"
  depends_on "libsixel"
  depends_on "openssl@1.1"
  depends_on "spdlog"
  depends_on "tbb"
  depends_on "vips"
  depends_on "zeromq"

  def install
    system "cmake", "-S", ".", "-B", "build",
                    "-DENABLE_X11=OFF",
                    "-DENABLE_OPENCV=OFF",
                    *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
    bin.install_symlink "ueberzug" => "ueberzugpp"
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
