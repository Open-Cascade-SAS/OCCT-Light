// samples/manual/cpp/hello_world.cpp — Minimal OCCT-Light C++ veneer sample.
//
// Build (from repo root, after cmake --build --preset minimal):
//   c++ -std=c++17 -I include/ -I build/minimal/generated/ \
//       samples/manual/cpp/hello_world.cpp \
//       -L build/minimal/lib -locctl -o samples/manual/cpp/hello_world

#include <occtl-hpp/bool.hpp>
#include <occtl-hpp/core.hpp>
#include <occtl-hpp/io_brep.hpp>
#include <occtl-hpp/prim.hpp>

#include <cstdlib>
#include <iostream>

int main()
{
  try
  {
    occtl::Runtime aRuntime; // one-time init + auto-shutdown

    occtl::Graph aGraph;

    const auto aBox1 = occtl::prim::make_box(aGraph, 10.0, 10.5, 10.0);
    std::cout << "Box 1: " << aBox1.get().bits << '\n';

    const auto aBox2 = occtl::prim::make_box(aGraph, 10.0, 10.0, 10.0);
    std::cout << "Box 2: " << aBox2.get().bits << '\n';

    const auto aFusedRoot = occtl::bool_::fuse(aGraph, {aBox1}, {aBox2});
    std::cout << "Fused root: " << aFusedRoot.get().bits << '\n';

    std::cout << "Writing fused.brep...\n";
    occtl::io_brep::write(aGraph, aFusedRoot, "fused.brep");
    std::cout << "Done.\n";

    return EXIT_SUCCESS;
  }
  catch (const occtl::Error& theError)
  {
    std::cerr << "OCCT-Light error: " << theError.what() << " (code=" << theError.code() << ")\n";
    return EXIT_FAILURE;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "Unexpected error: " << ex.what() << '\n';
    return EXIT_FAILURE;
  }
}
