/***
 *
 * NNPDF C++ Driver
 *
 * Stefano Carrazza for the NNPDF Collaboration
 * email: stefano.carrazza@mi.infn.it
 *
 * December 2014
 *
 * Usage:
 *
 *  NNPDFDriver *pdf = new NNPDFDriver("gridname.LHgrid");
 *
 *  pdf->initPDF(0); // select replica [0,fMem]
 *
 *  or 
 * 
 *  NNPDFDriver *pdf = new NNPDFDriver("gridname.LHgrid", 0);
 *
 *  then
 *
 *  pdf->xfx(x,Q,fl); // -> returns double
 *
 *  // with fl = [-6,7], LHAPDF format
 *
 */

#pragma once

#include <array>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>
using std::string;
using std::vector;

class NNPDFDriver {

 private:

  // Interpolation order
  static constexpr int fM = 4;
  static constexpr int fN = 4;
  inline static constexpr std::array<int, 13> fFlavors{-6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6 };
  static constexpr std::size_t fFlavorCount = fFlavors.size();

  struct InterpolationCoefficients {
    std::array<std::array<double, fN>, fM> q2;
  };

  struct BivariateInterpolationCoefficients {
    std::array<std::array<double, fN>, fM> xy;
  };

  using FlavorInterpolationCoefficients = std::array<InterpolationCoefficients, fFlavorCount>;
  using FlavorBivariateInterpolationCoefficients = std::array<BivariateInterpolationCoefficients, fFlavorCount>;
  using FlavorGridValues = std::array<std::array<std::array<double, fN>, fM>, fFlavorCount>;

  std::unordered_map<std::size_t, FlavorBivariateInterpolationCoefficients> fCache{};
  std::size_t fCacheQ2Stride = 0;

  int fNFL;           //! Total flavour number
  int fNX;            //! Total number of x points in the grid
  vector<int> fNQ2;   //! Total number of Q2 points in the grid (subgrids)
  int fMem;           //! Total number of Members
  int fRep;           //! Select the current replica
  double fAlphas;     //! AlphaS value
  double fXMinGrid;   //! Minimum size of the grid
  double *fXGrid;     //! x grid
  double *fLogXGrid;  //! x grid
  vector<double*>     fQ2Grid;    //! q2 grid
  vector<double*>     fLogQ2Grid; //! q2 grid
  vector<double****>  fPDFGrid;   //! PDF grid
  bool fHasPhoton;    //! bool with photon information
  bool fSingleMem;    //! bool which determines the constructor
  bool fLHAPDF6;      //! bool which determines the grid version
  
 public:
  /// The constructor
  NNPDFDriver(string const& gridfilename = "", int const& rep = -1);
  /// The destructor
  ~NNPDFDriver();

  //! Init PDF member irep = [0, fMem]
  void initPDF(int irep);

  //! returns the x*pdf of flavour id = [-6,7], LHA order.
  double xfx(double const& X, double const& Q, int const& ID);

  //! Get NFL method, returns total number of flavours
  int GetNFL() { return fNFL; }

  //! Returns the flavours in LHA order
  const std::array<int, fFlavorCount>& GetFlavors() const
  {
    return fFlavors;
  }

  //! Get AlphaS method, returns the alphas at Mz
  double GetAlphaSMz() { return fAlphas; }

  //! Returns true if the set contains the photon PDF
  bool hasPhoton() { return fHasPhoton; }

  //! Return the total number of replicas
  int GetMembers() { return fMem+1; }

 private:
  /// Reads the PDF from file
  void readPDFSet(string const&, int const&);
  FlavorBivariateInterpolationCoefficients lh_polin2_bivariate_coefficients_batch( const double[], const double[], const FlavorGridValues&);
  double lh_polin2_bivariate_evaluate(const BivariateInterpolationCoefficients&, double, double);
  template <int N> std::array<double, N> lh_polint_coefficients(const double[], const double[]);
  void lh_polint_coefficients_batch( const double[], const FlavorGridValues&, int, FlavorInterpolationCoefficients&);
};

  
