#include "NNPDFDriver.h"
#include "LHAPDF/LHAPDF.h"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <fstream>

namespace
{
using Clock = std::chrono::steady_clock;

double elapsedSeconds(Clock::time_point start, Clock::time_point finish)
{
  return std::chrono::duration<double>(finish - start).count();
}

long residentMemoryKb()
{
  std::ifstream status("/proc/self/status");
  std::string name;
  long memoryKb = 0;
  while (status >> name)
    {
      if (name == "VmRSS:")
        {
          status >> memoryKb;
          break;
        }
      status.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
  return memoryKb;
}
}

int main(int argc, char** argv)
{
  if (argc < 3)
    {
      std::cerr << "usage: ./test1 <gridname> <member> [iterations]" << std::endl;
      return EXIT_FAILURE;
    }

  const std::string gridname = argv[1];
  const int member = std::atoi(argv[2]);
  const int iterations = argc > 3 ? std::atoi(argv[3]) : 10000;
  if (iterations <= 0)
    {
      std::cerr << "iterations must be positive" << std::endl;
      return EXIT_FAILURE;
    }

  const double x[] = {1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2,
                      1e-1, 3e-1, 5e-1, 7e-1, 9e-1};
  const double q[] = {1.0, 2.0, 10.0, 100.0, 1000.0, 10000.0};
  const int flavors[] = {-6, -5, -4, -3, -2, -1, 0,
                         1, 2, 3, 4, 5, 6};

  const long memoryBeforeCppKb = residentMemoryKb();



  volatile double cppSum = 0.0;


  Clock::time_point cppStart;
  NNPDFDriver* nnpdf;
  {
  nnpdf = new NNPDFDriver(gridname, member);
  cppStart = Clock::now();
  for (int iteration = 0; iteration < iterations; ++iteration)
    for (int flavor : flavors)
      for (double scale : q)
        for (double momentum : x)
          cppSum += nnpdf->xfx(momentum, scale, flavor);

}
const auto cppFinish = Clock::now();
const long memoryAfterCppKb = residentMemoryKb();
delete nnpdf;


const long memoryBeforeLhapdfKb = residentMemoryKb();
  volatile double lhapdfSum = 0.0;
  LHAPDF::PDF* pdf;
  Clock::time_point lhapdfStart;
{
 pdf = LHAPDF::mkPDF(gridname, member);
 lhapdfStart = Clock::now();
    for (int iteration = 0; iteration < iterations; ++iteration)
    for (int flavor : flavors)
      for (double scale : q)
        for (double momentum : x)
          lhapdfSum += pdf->xfxQ(flavor, momentum, scale);
}
    const auto lhapdfFinish = Clock::now();
  const long memoryAfterLhapdfKb = residentMemoryKb();
    delete pdf;

  const long long calls = static_cast<long long>(iterations)
    * (sizeof(flavors) / sizeof(flavors[0]))
    * (sizeof(q) / sizeof(q[0]))
    * (sizeof(x) / sizeof(x[0]));
  const double cppSeconds = elapsedSeconds(cppStart, cppFinish);
  const double lhapdfSeconds = elapsedSeconds(lhapdfStart, lhapdfFinish);

  std::cout << std::setprecision(6) << std::fixed;
  std::cout << "Calls: " << calls << std::endl;
  std::cout << "Memory: baseline " << memoryBeforeCppKb << " kB, C++ +"
            << memoryAfterCppKb - memoryBeforeCppKb << " kB, LHAPDF +"
            << memoryAfterLhapdfKb - memoryBeforeLhapdfKb << " kB"
            << std::endl;
  std::cout << "C++:    " << cppSeconds << " s ("
            << cppSeconds / calls * 1e9 << " ns/call, sum " << cppSum << ")"
            << std::endl;
  std::cout << "LHAPDF: " << lhapdfSeconds << " s ("
            << lhapdfSeconds / calls * 1e9 << " ns/call, sum " << lhapdfSum << ")"
            << std::endl;

  return EXIT_SUCCESS;
}