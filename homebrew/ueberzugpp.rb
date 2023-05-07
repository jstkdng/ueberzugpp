class Ueberzugpp < Formula
  desc "Drop in replacement for ueberzug written in C++"
  homepage "https://github.com/jstkdng/ueberzugpp"
  url "https://github.com/jstkdng/ueberzugpp/archive/refs/tags/v2.8.0.tar.gz"
  sha256 "3f7e21052c3c218c436b1d2d8aedc1f02573ad83046be00b05069bfdc29bf4e2"
  license "GPL3"

  depends_on "cmake" => :build
  depends_on "cppzmq" => :build
  depends_on "cli11" => :build
  depends_on "nlohmann-json" => :build
  depends_on "pkg-config" => :build
  depends_on "spdlog"
  depends_on "libvips"
  depends_on "libsixel"
  depends_on "zeromq"
  depends_on "openssl"
  depends_on "tbb"
  depends_on "fmt"

  def install
    system "cmake", "-S", ".", "-B", "build", "-DCMAKE_BUILD_TYPE=Release", "-Wno-dev",
      "-DCMAKE_INSTALL_PREFIX='#{ENV["prefix"]}'",
      "-DENABLE_X11=OFF", "-DENABLE_OPENCV=OFF", "-DOPENSSL_ROOT_DIR=#{ENV["HOMEBREW_PREFIX"]}/opt/openssl", *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
    bin.install "build/ueberzug" => "ueberzugpp"
  end

  test do
    system "ueberzugpp", "-V"
  end
end
