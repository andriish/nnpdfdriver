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

#include "NNPDFDriver.h"
#include <fstream>
#include <stdlib.h>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cmath>

#define NNDriverVersion "1.0.8"

using namespace std;

void split(vector<string>& results, string const& input)
{
  stringstream strstr(input);
  std::istream_iterator<string> it(strstr);
  std::istream_iterator<string> end;
  results.assign(it, end);
  return;
}

/**
 * @brief Default constructor
 * @param gridfilename the string containing the LHgrid file name and location.
 */
NNPDFDriver::NNPDFDriver(string const& gridfilename, int const& rep):
  fNFL(13),
  fNX(100),
  fMem(1),
  fRep(0),
  fAlphas(0),
  fXMinGrid(1e-9),
  fXGrid(NULL),
  fLogXGrid(NULL),
  fHasPhoton(false),
  fSingleMem(false),
  fLHAPDF6(false)
{
  // Logo
  cout << " ****************************************" << endl;
  cout << "      NNPDFDriver version " << NNDriverVersion << endl;
  cout << "  Grid: " << gridfilename << endl;
  cout << " ****************************************" << endl;

  // Check the grid version
  if (gridfilename.find(".LHgrid") == string::npos) fLHAPDF6 = true;

  // Read PDFs from file
  readPDFSet(gridfilename,rep);
}

/**
 * @brief NNPDFDriver::~NNPDFDriver the destructor
 */
NNPDFDriver::~NNPDFDriver()
{  
  for (size_t s = 0; s < fPDFGrid.size(); s++)
    for (int imem = 0; imem <= fMem; imem++)
      {
	for (int i = 0; i < fNFL; i++)
	  {
	    for (int j = 0; j < fNX; j++)
	      if (fPDFGrid[s][imem][i][j]) delete[] fPDFGrid[s][imem][i][j];
	    if (fPDFGrid[s][imem][i]) delete[] fPDFGrid[s][imem][i];
	  }
	if (fPDFGrid[s][imem]) delete[] fPDFGrid[s][imem];
      }
  fPDFGrid.clear();

  if (fXGrid) delete[] fXGrid;
  if (fLogXGrid) delete[] fLogXGrid;

  for (size_t t = 0; t < fQ2Grid.size(); t++)
    if (fQ2Grid[t]) delete[] fQ2Grid[t];
  fQ2Grid.clear();

  for (size_t t = 0; t < fLogQ2Grid.size(); t++)
    if (fLogQ2Grid[t]) delete[] fLogQ2Grid[t];  
  fLogQ2Grid.clear();

  fNQ2.clear();
}

/**
 * @brief NNPDFDriver::initPDF
 * @param irep
 */
void NNPDFDriver::initPDF(int irep)
{
  if (fSingleMem)
    {
      cout << "Error: initPDF not available due to the constructor" << endl;
      exit(-1);
    }
  else
    {      
      if (irep > fMem || irep < 0)
	{
	  cout << "Error: replica out of range [0," << fMem << "]" << endl;
	  exit(-1);
	}
      else
	fRep = irep;
    }
}

/**
 * @brief NNPDFDriver::readPDFSet read the LHgrid file into an array
 * @param grid the LHgrid filename
 */
void NNPDFDriver::readPDFSet(string const& grid, int const& rep)
{
  if (fLHAPDF6)
    {
      fstream f;
      stringstream file("");
      int firstindex = (int) grid.find_last_of("/") + 1;
      int lastindex  = (int) grid.length() - firstindex;
      
      string name = grid.substr(firstindex, lastindex);      
      file << grid << "/" << name << ".info";
      f.open(file.str().c_str(), ios::in);
      
      if (f.fail())
	{
	  cout << "Error: cannot open file " << grid << endl;
	  exit(-1);
	}

      string tmp;
      vector<string> splitstring;	      
      for (;;)
	{
	  getline(f,tmp);

	  if (tmp.find("SetDesc:") != string::npos)
	    cout << tmp << endl;
	  
	  if (tmp.find("NumMembers:") != string::npos)
	    {
	      split(splitstring,tmp);
	      fMem = atof(splitstring[1].c_str())-1;
	    }
	  
	  if (tmp.find("Flavors: [") != string::npos)
	    if (tmp.find("22") != string::npos) { fHasPhoton = true;  fNFL++; }

	  if (tmp.find("AlphaS_MZ:") != string::npos)
	    {
	      split(splitstring,tmp);
	      fAlphas = atof(splitstring[1].c_str());
	    }

	  if (tmp.find("XMin:") != string::npos)
	    {
	      split(splitstring,tmp);
	      
	      // only for LHAPDF6 grids
	      fXMinGrid = atof(splitstring[1].c_str());
	    }
	  
	  if (f.eof()) break;
	}
      f.close();

      // single member switcher
      if (rep >= 0) { fMem = 0; fSingleMem = true; }

      if (fSingleMem)
	{
	  stringstream file("");
	  if (rep < 10)
	    file << grid << "/" << name << "_000" << rep << ".dat";
	  else if (rep < 100)
	    file << grid << "/" << name << "_00" << rep << ".dat";
	  else if (rep < 1000)
	    file << grid << "/" << name << "_0" << rep << ".dat";
	  else
	    file << grid << "/" << name << "_" << rep << ".dat";

	  f.open(file.str().c_str(), ios::in);
	  
	  getline(f, tmp);
	  getline(f, tmp);
	  getline(f, tmp);

	  int sub = 0;
	  for (;;)
	    {	      
	      if (sub == 0)
		{
		  // reading Xgrid
		  getline(f, tmp);
		  split(splitstring,tmp);
	      
		  fNX = splitstring.size();
		  fXGrid = new double[fNX];
		  fLogXGrid = new double[fNX];
		  for (int ix = 0; ix < fNX; ix++)
		    {
		      fXGrid[ix] = atof(splitstring[ix].c_str());
		      fLogXGrid[ix] = log(fXGrid[ix]);
		    }
		}

	      getline(f, tmp);
	      split(splitstring, tmp);
	      
	      fNQ2.push_back(splitstring.size());	    
	      fQ2Grid.push_back(new double[fNQ2[sub]]);
	      fLogQ2Grid.push_back(new double[fNQ2[sub]]);

	      for (int iq = 0; iq < fNQ2[sub]; iq++)
		{
		  fQ2Grid[sub][iq] = pow(atof(splitstring[iq].c_str()), 2.0);
		  fLogQ2Grid[sub][iq] = log(fQ2Grid[sub][iq]);	       
		}

	      // skip flavor line
	      getline(f, tmp);
	      vector<int> fls;
	      
	      split(splitstring,tmp);
	      for (int i = 0; i < splitstring.size(); i++)
		{	      
		  if (atoi(splitstring[i].c_str()) == 21)
		    fls.push_back(6);
		  else if (atoi(splitstring[i].c_str()) == 22)
		    fls.push_back(13);
		  else
		    fls.push_back(atoi(splitstring[i].c_str())+6);
		}

	      // building PDFgrid
	      fPDFGrid.push_back(new double***[fMem+1]);     
	      for (int imem = 0; imem <= fMem; imem++)
		{
		  fPDFGrid[sub][imem] = new double**[fNFL];
		  for (int i = 0; i < fNFL; i++)
		    {
		      fPDFGrid[sub][imem][i] = new double*[fNX];
		      for (int j = 0; j < fNX; j++)
			{
			  fPDFGrid[sub][imem][i][j] = new double[fNQ2[sub]];
			  for (int z = 0; z < fNQ2[sub]; z++)
			    fPDFGrid[sub][imem][i][j][z] = 0.0;
			}
		    }
		}
	      
	      // read PDF grid points	      
	      for (int imem = 0; imem <= fMem; imem++)
		for (int ix = 0; ix < fNX; ix++)
		  for (int iq = 0; iq < fNQ2[sub]; iq++)
		    for (int fl = 0; fl < fls.size(); fl++) 
		      f >> fPDFGrid[sub][imem][fls[fl]][ix][iq];	

	      getline(f, tmp);
	      getline(f, tmp);

	      if (tmp.find("---") != string::npos)
		{
		  sub++;
		  getline(f, tmp);
		  if (f.eof()) break;
		  continue;
		}
	    }
	  
	  f.close();
	}
      else
	{
	  for (int imem = 0; imem <= fMem; imem++)
	    {
	      stringstream file("");
	      if (imem < 10)
		file << grid << "/" << name << "_000" << imem << ".dat";
	      else if (imem < 100)
		file << grid << "/" << name << "_00" << imem << ".dat";
	      else if (imem < 1000)
		file << grid << "/" << name << "_0" << imem << ".dat";
	      else
		file << grid << "/" << name << "_" << imem << ".dat";

	      f.open(file.str().c_str(), ios::in);
	  
	      getline(f, tmp);
	      getline(f, tmp);
	      getline(f, tmp);
	      
	      int sub = 0;
	      for (;;)
		{	      
		  if (sub == 0 && imem == 0)
		    {
		      // reading Xgrid
		      getline(f, tmp);
		      split(splitstring,tmp);
		      
		      fNX = splitstring.size();
		      fXGrid = new double[fNX];
		      fLogXGrid = new double[fNX];
		      for (int ix = 0; ix < fNX; ix++)
			{
			  fXGrid[ix] = atof(splitstring[ix].c_str());
			  fLogXGrid[ix] = log(fXGrid[ix]);
			}
		    }
		  else if (sub == 0) getline(f,tmp);
		  
		  if (imem == 0)
		    {
		      getline(f, tmp);
		      split(splitstring, tmp);
		      
		      fNQ2.push_back(splitstring.size());	    
		      fQ2Grid.push_back(new double[fNQ2[sub]]);
		      fLogQ2Grid.push_back(new double[fNQ2[sub]]);
		  
		      for (int iq = 0; iq < fNQ2[sub]; iq++)
			{
			  fQ2Grid[sub][iq] = pow(atof(splitstring[iq].c_str()), 2.0);
			  fLogQ2Grid[sub][iq] = log(fQ2Grid[sub][iq]);	       
			}

		      fPDFGrid.push_back(new double***[fMem+1]);     
		    }
		  else getline(f, tmp);

		  // skip flavor line
		  getline(f, tmp);
		  vector<int> fls;
		  
		  split(splitstring,tmp);
		  for (int i = 0; i < splitstring.size(); i++)
		    {	      
		      if (atoi(splitstring[i].c_str()) == 21)
			fls.push_back(6);
		      else
			fls.push_back(atoi(splitstring[i].c_str())+6);
		    }

		  // building PDFgrid
		  fPDFGrid[sub][imem] = new double**[fNFL];
		  for (int i = 0; i < fNFL; i++)
		    {
		      fPDFGrid[sub][imem][i] = new double*[fNX];
		      for (int j = 0; j < fNX; j++)
			{
			  fPDFGrid[sub][imem][i][j] = new double[fNQ2[sub]];
			  for (int z = 0; z < fNQ2[sub]; z++)
			    fPDFGrid[sub][imem][i][j][z] = 0.0;
			}
		    }
		  
		  // read PDF grid points	      
		  for (int ix = 0; ix < fNX; ix++)
		    for (int iq = 0; iq < fNQ2[sub]; iq++)
		      for (int fl = 0; fl < fls.size(); fl++) 
			f >> fPDFGrid[sub][imem][fls[fl]][ix][iq];	
		  
		  getline(f, tmp);
		  getline(f, tmp);
		  
		  if (tmp.find("---") != string::npos)
		    {
		      sub++;
		      getline(f, tmp);
		      if (f.eof()) break;
		      continue;
		    }
		}
	      
	      f.close();	      
	    }
	}

    }
  else
    {
      // reading Q2 and setting subgrids to 0 because it is a LHAPDF5 grid
      int sub = 0;
      
      fstream f;
      f.open(grid.c_str(),ios::in);
      if (f.fail())
	{
	  cout << "Error: cannot open file " << grid << endl;
	  exit(-1);
	}
      
      // Read header
      string tmp;
      for (;;) {
	getline(f,tmp);
	if (tmp.find("Alphas") != string::npos) break;
	cout << tmp << endl;
      }
      cout << endl;
      
      // Extract alphas and total number of members
      for (;;) {
	getline(f,tmp);
	if (tmp.find("Parameterlist:") != string::npos){
	  f >> tmp >> fMem >> tmp >> tmp >> fAlphas;
	  break;
        }
      }
      
      // Reading grid: removing header
      for (;;)
	{
	  getline(f,tmp);
	  if (tmp.find("NNPDF20intqed") != string::npos)
	    {
	      fHasPhoton = true;
	      fNFL++;
	      getline(f,tmp);
	      break;
	    }
	  else if (tmp.find("NNPDF20int") != string::npos)
	    {
	      getline(f,tmp);
	      break;
	    }
	}

      // getting nx
      f >> fNX;
      fXGrid = new double[fNX];
      for (int ix = 0; ix < fNX; ix++)
	f >> fXGrid[ix];
      
      fLogXGrid = new double[fNX];
      for (int ix = 0; ix < fNX; ix++)
	fLogXGrid[ix] = log(fXGrid[ix]);
      
      fNQ2.push_back(50);
      f >> fNQ2[sub];
      
      f >> tmp; // skip first value
      fQ2Grid.push_back(new double[fNQ2[sub]]);
      for (int iq = 0; iq < fNQ2[sub]; iq++)
	f >> fQ2Grid[sub][iq];
      
      fLogQ2Grid.push_back(new double[fNQ2[sub]]);
      for (int iq = 0; iq < fNQ2[sub]; iq++)
	fLogQ2Grid[sub][iq] = log(fQ2Grid[sub][iq]);
      
      // Prepare grid array
      if (rep >= 0) { fMem = 0; fSingleMem = true; }
      
      fPDFGrid.push_back(new double***[fMem+1]);
      for (int imem = 0; imem <= fMem; imem++)
	{
	  fPDFGrid[sub][imem] = new double**[fNFL];
	  for (int i = 0; i < fNFL; i++)
	    {
	      fPDFGrid[sub][imem][i] = new double*[fNX];
	      for (int j = 0; j < fNX; j++)
		{
		  fPDFGrid[sub][imem][i][j] = new double[fNQ2[sub]];
		  for (int z = 0; z < fNQ2[sub]; z++)
		    fPDFGrid[sub][imem][i][j][z] = 0.0;
		}
	    }
	}
      
      if (fSingleMem)
	{
	  // skip replicas before reading
	  for (int imem = 0; imem < rep; imem++)
	    for (int ix = 0; ix < fNX; ix++)
	      for (int iq = 0; iq < fNQ2[sub]; iq++)
		for (int fl = 0; fl < fNFL; fl++) 
		  f >> tmp;	      
	}
      
      // read PDF grid points
      f >> tmp; // remove replica number
      for (int imem = 0; imem <= fMem; imem++)
	for (int ix = 0; ix < fNX; ix++)
	  for (int iq = 0; iq < fNQ2[sub]; iq++)
	    for (int fl = 0; fl < fNFL; fl++) 
	      f >> fPDFGrid[sub][imem][fl][ix][iq];	
      
      f.close();
    }
}

/**
 * @brief NNPDFDriver::xfx
 * @param x
 * @param Q
 * @param id
 * @return
 */
double NNPDFDriver::xfx(double const&X, double const& Q, int const& ID)
{
  double res= 0;
  double Q2 = Q*Q;
  double x  = X;

  const int id    = ID+6;
  int sub   = 0;
  if (fLHAPDF6)
    {
	const vector<double*>::const_iterator nextSubgrid = std::upper_bound(
	  fQ2Grid.begin(), fQ2Grid.end(), Q2,
	  [](double value, const double* grid) { return value < grid[0]; });
	if (nextSubgrid != fQ2Grid.begin())
	  sub = static_cast<int>(nextSubgrid - fQ2Grid.begin()) - 1;
    }

	// check bounds
	const double* const xGrid = fXGrid;
	const double* const q2Grid = fQ2Grid[sub];
	const int nq2 = fNQ2[sub];
	const double xMin = std::max(fXMinGrid, xGrid[0]);
	const double xMax = xGrid[fNX - 1];
	if (x < xMin || x > xMax) [[unlikely]] {
    cout << "Parton interpolation: x out of range -- freezed" << endl;  
		x = std::clamp(x, xMin, xMax);
  }
	const double q2Min = q2Grid[0];
	const double q2Max = q2Grid[nq2 - 1];
	if (Q2 < q2Min || Q2 > q2Max) [[unlikely]] {
    cout << "Parton interpolation: Q2 out of range -- freezed" << endl;
		cout << Q2 << "\t" << q2Min << "\t" << sub << endl;
		Q2 = std::clamp(Q2, q2Min, q2Max);
  }

	// Find the last grid point less than or equal to the query.
	const int ix = static_cast<int>(std::upper_bound(fXGrid, fXGrid + fNX, x) - fXGrid) - 1;
	const int iq2 = static_cast<int>(std::upper_bound(fQ2Grid[sub], fQ2Grid[sub] + fNQ2[sub], Q2) - fQ2Grid[sub]) - 1;

	/* Previous hand-written binary searches:
	int minx = 0;
	int maxx = fNX;
	while (maxx-minx > 1)
		{
			int midx = (minx+maxx)/2;
			if (x < fXGrid[midx])
				maxx = midx;
			else
				minx = midx;
		}
	int ix = minx;

	int minq = 0;
	int maxq = fNQ2[sub];
	while (maxq-minq > 1)
		{
			int midq = (minq+maxq)/2;
			if (Q2 < fQ2Grid[sub][midq])
				maxq = midq;
			else
				minq = midq;
		}
	int iq2 = minq;
	*/
#define MULTIFLAVOUR
  // Assign grid for interpolation. M,N -> order of polyN interpolation
  double x1a[fM], x2a[fN];
  double ya[fM][fN];

	const int ixStart = ix+1 < fM/2 ? 0
		: ix+1 > fNX-fM/2 ? fNX-fM
		: ix+1-fM/2;
	const int iq2Start = iq2+1 < fN/2 ? 0
		: iq2+1 > fNQ2[sub]-fN/2 ? fNQ2[sub]-fN
		: iq2+1-fN/2;

  // define points where to evaluate interpolation
  // choose between linear or logarithmic (x,Q2) interpolation
  constexpr double xch = 1e-1;

	const double x1 = x < xch ? log(x) : x;
	const double x2 = log(Q2);
  
  if (id < 0 || id > fNFL-1)
    {
      cout << "Error: flavor out of range" << endl;
      exit(3);
    }
#ifdef MULTIFLAVOUR
	const std::array<int, 5> cacheKey = {ixStart, iq2Start, sub, fRep, -1};
#else
	const std::array<int, 5> cacheKey = {ixStart, iq2Start, sub, fRep, id};
      // Choose betwen linear or logarithmic (x,Q2) interpolation
	double** const flavorGrid = fPDFGrid[sub][fRep][id];

#endif
	auto cached = fCache.find(cacheKey);
	const bool isCached = (cached != fCache.end());


	#ifdef MULTIFLAVOUR
	auto*  flavorGridall = fPDFGrid[sub][fRep];
	constexpr std::size_t flavorCount = 13;
	const auto& flavours = GetFlavors();
	//if (fHasPhoton) flavours.push_back(7);
	const std::size_t selectedFlavorIndex =
	  static_cast<std::size_t>(id);
	FlavorGridValues yaall;
	#endif

	const double* const logQ2Grid = fLogQ2Grid[sub];
      if (x < xch)
	{
	  for (int i = 0; i < fM; i++)
	    {
	      const int xIndex = ixStart + i;
	      x1a[i] = fLogXGrid[xIndex];
	      if (isCached) continue;
	      for (int j = 0; j < fN; j++)
		{
		  const int qIndex = iq2Start + j;
		  x2a[j] = logQ2Grid[qIndex];
#ifdef MULTIFLAVOUR		  
			for (size_t flavorIndex = 0; flavorIndex < flavorCount; flavorIndex++)
            {
							const int gridFlavor = flavours[flavorIndex] + 6;
							yaall[flavorIndex][i][j] = flavorGridall[gridFlavor][xIndex][qIndex];
            }
#else

           ya[i][j] = flavorGrid[xIndex][qIndex];
#endif
		}
	    }
	}
	else
	{
	  for (int i = 0; i < fM; i++)
	    {
	      const int xIndex = ixStart + i;
	      x1a[i] = fXGrid[xIndex];
	      if (isCached) continue;
	      for (int j = 0; j < fN; j++)
		{
		  const int qIndex = iq2Start + j;
		  x2a[j] = logQ2Grid[qIndex];
#ifdef MULTIFLAVOUR		  
			for (size_t flavorIndex = 0; flavorIndex < flavorCount; flavorIndex++)
            {
							const int gridFlavor = flavours[flavorIndex] + 6;
							yaall[flavorIndex][i][j] = flavorGridall[gridFlavor][xIndex][qIndex];
            }
#else

           ya[i][j] = flavorGrid[xIndex][qIndex];
#endif
		}
	    }
	}
      
      // 2D polynomial interpolation
	//double y = 0;
	/*
	{
	double y = lh_polin2_coefficients(x1a, x2a, ya, x1, x2);
	res = y;
	}
	*/



	
#ifdef MULTIFLAVOUR	
	if (cached != fCache.end())
		{
			res = lh_polin2_bivariate_evaluate(
				cached->second[selectedFlavorIndex], x1, x2);
		}
	else
		{
			const FlavorBivariateInterpolationCoefficients coefficients =
				lh_polin2_bivariate_coefficients_batch(x1a, x2a, yaall);
			res = lh_polin2_bivariate_evaluate(
				coefficients[selectedFlavorIndex], x1, x2);
			fCache.emplace(cacheKey, coefficients);
		}
#else



	
	


	if (isCached)
		{
			res = lh_polin2_bivariate_evaluate(
				cached->second[id], x1, x2);
		}
	else
		{
			const BivariateInterpolationCoefficients coefficients =
				lh_polin2_bivariate_coefficients(x1a, x2a, ya);
			res = lh_polin2_bivariate_evaluate(coefficients, x1, x2);
			FlavorBivariateInterpolationCoefficients cachedCoefficients{};
			cachedCoefficients[id] = coefficients;
			fCache.emplace(cacheKey, cachedCoefficients);
		}

#endif


  return res;
}

double NNPDFDriver::lh_polin2(const double x1a[], const double x2a[],
			    const double ya[][fN],
			    double x1, double x2)
{
  double ymtmp[fM];
//#pragma GCC unroll 4
  for (int j = 0; j < fM; j++)
    {
	  ymtmp[j] = lh_polint<fN>(x2a,ya[j],x2);
    }
	return lh_polint<fM>(x1a,ymtmp,x1);
}

double NNPDFDriver::lh_polin2_coefficients(
	const double x1a[], const double x2a[], const double ya[][fN],
	double x1, double x2)
{
	double ymtmp[fM];
	for (int j = 0; j < fM; j++)
		{
			const std::array<double, fN> coefficients = lh_polint_coefficients<fN>(x2a, ya[j]);
			ymtmp[j] = coefficients[fN - 1];
			for (int k = fN - 2; k >= 0; k--) ymtmp[j] = ymtmp[j] * x2 + coefficients[k];
		}
	const std::array<double, fM> coefficients = lh_polint_coefficients<fM>(x1a, ymtmp);
	double y = coefficients[fM - 1];
	for (int k = fM - 2; k >= 0; k--) y = y * x1 + coefficients[k];
	return y;
}

double NNPDFDriver::lh_polin2_coefficients_hold(
	const double x1a[], const double x2a[], const double ya[][fN],
	double x1, double x2, InterpolationCoefficients& heldCoefficients, bool recalculate)
{
	double ymtmp[fM];
	if (recalculate) {
	for (int j = 0; j < fM; j++)
		{
			heldCoefficients.q2[j] = lh_polint_coefficients<fN>(x2a, ya[j]);
		}
	} 
	for (int j = 0; j < fM; j++)
		{
			ymtmp[j] = heldCoefficients.q2[j][fN - 1];
			for (int k = fN - 2; k >= 0; k--)
				ymtmp[j] = ymtmp[j] * x2 + heldCoefficients.q2[j][k];
		}
	const std::array<double, fM> coefficients = lh_polint_coefficients<fM>(x1a, ymtmp);
	double y = coefficients[fM - 1];
	for (int k = fM - 2; k >= 0; k--) y = y * x1 + coefficients[k];
	return y;
}

double NNPDFDriver::lh_polin2_coefficients_hold_batch(
	const double x1a[], const double x2a[],
	const FlavorGridValues& yaall,
	double x1, double x2, FlavorInterpolationCoefficients& heldCoefficients,
	std::size_t selectedFlavorIndex)
{
	for (int j = 0; j < fM; j++)
		lh_polint_coefficients_batch(x2a, yaall, j, heldCoefficients);

	const InterpolationCoefficients& selectedCoefficients =
		heldCoefficients[selectedFlavorIndex];
	double ymtmp[fM];
	for (int j = 0; j < fM; j++)
		{
			ymtmp[j] = selectedCoefficients.q2[j][fN - 1];
			for (int k = fN - 2; k >= 0; k--)
				ymtmp[j] = ymtmp[j] * x2 + selectedCoefficients.q2[j][k];
		}
	const std::array<double, fM> coefficients = lh_polint_coefficients<fM>(x1a, ymtmp);
	double y = coefficients[fM - 1];
	for (int k = fM - 2; k >= 0; k--) y = y * x1 + coefficients[k];
	return y;
}

NNPDFDriver::BivariateInterpolationCoefficients
NNPDFDriver::lh_polin2_bivariate_coefficients(
	const double x1a[], const double x2a[], const double ya[][fN])
{
	std::array<std::array<double, fN>, fM> q2Coefficients;
	for (int xIndex = 0; xIndex < fM; xIndex++)
		q2Coefficients[xIndex] =
			lh_polint_coefficients<fN>(x2a, ya[xIndex]);

	BivariateInterpolationCoefficients coefficients{};
	for (int q2Power = 0; q2Power < fN; q2Power++)
		{
			double values[fM];
			for (int xIndex = 0; xIndex < fM; xIndex++)
				values[xIndex] = q2Coefficients[xIndex][q2Power];

			const auto xCoefficients =
				lh_polint_coefficients<fM>(x1a, values);
			for (int xPower = 0; xPower < fM; xPower++)
				coefficients.xy[xPower][q2Power] = xCoefficients[xPower];
		}
	return coefficients;
}

NNPDFDriver::FlavorBivariateInterpolationCoefficients
NNPDFDriver::lh_polin2_bivariate_coefficients_batch(
	const double x1a[], const double x2a[], const FlavorGridValues& yaall)
{
	FlavorInterpolationCoefficients q2Coefficients{};
	for (int xIndex = 0; xIndex < fM; xIndex++)
		lh_polint_coefficients_batch(
			x2a, yaall, xIndex, q2Coefficients);

	FlavorBivariateInterpolationCoefficients coefficients{};
	for (std::size_t flavor = 0; flavor < fFlavorCount; flavor++)
		for (int q2Power = 0; q2Power < fN; q2Power++)
			{
				double values[fM];
				for (int xIndex = 0; xIndex < fM; xIndex++)
					values[xIndex] =
						q2Coefficients[flavor].q2[xIndex][q2Power];

				const auto xCoefficients =
					lh_polint_coefficients<fM>(x1a, values);
				for (int xPower = 0; xPower < fM; xPower++)
					coefficients[flavor].xy[xPower][q2Power] =
						xCoefficients[xPower];
			}
	return coefficients;
}

double NNPDFDriver::lh_polin2_bivariate_evaluate(
	const BivariateInterpolationCoefficients& coefficients,
	double x1, double x2)
{
	double y = 0.0;
	for (int q2Power = fN - 1; q2Power >= 0; q2Power--)
		{
			double xPolynomial = coefficients.xy[fM - 1][q2Power];
			for (int xPower = fM - 2; xPower >= 0; xPower--)
				xPolynomial = xPolynomial * x1
					+ coefficients.xy[xPower][q2Power];
			y = y * x2 + xPolynomial;
		}
	return y;
}


template <int N>
double NNPDFDriver::lh_polint(const double xa[], const double ya[], double x)
{
  int ns = 0;  
	double dy;
  double dif = abs(x-xa[0]);
  double c[fM > fN ? fM : fN];
  double d[fM > fN ? fM : fN];
//  #pragma GCC unroll 4
	for (int i = 0; i < N; i++)
    {
      const double dift = abs(x-xa[i]);
      if (dift < dif)
	{
	  ns = i;
	  dif = dift;
	}
      c[i] = ya[i];
      d[i] = ya[i];
    }
	double y = ya[ns];
  ns--;
//  #pragma GCC unroll 4
	for (int m = 1; m < N; m++)
    {
			for (int i = 0; i < N-m; i++)
	{
	  //const double ho = xa[i]-x;
	  //const double hp = xa[i+m]-x;
	  //const double w = c[i+1]-d[i];
	  //double den = ho-hp;

	  if (xa[i] == xa[i+m])[[unlikely]]
	    {
	      cout << "failure in polint" << endl;
	      exit(4);	       
	    }

		// den = w/den;
	  d[i] = (xa[i+m]-x)*(c[i+1]-d[i])/(xa[i]-xa[i+m]);
	  c[i] = d[i];
	}
	  if (2*(ns+1) < N-m)
	dy = c[ns+1];
      else {
	dy = d[ns];
	ns--;
      }
      y+=dy;
    }
	return y;
}

template <int N>
std::array<double, N> NNPDFDriver::lh_polint_coefficients(
	const double xa[], const double ya[])
{
	std::array<double, N> dividedDifferences;
	for (int i = 0; i < N; i++)
		dividedDifferences[i] = ya[i];

	for (int order = 1; order < N; order++)
		for (int i = N - 1; i >= order; i--)
			{
				const double denominator = xa[i] - xa[i - order];
				if (denominator == 0.0) [[unlikely]]
					{
						cout << "failure in polint" << endl;
						exit(4);
					}
				dividedDifferences[i] =
					(dividedDifferences[i] - dividedDifferences[i - 1]) / denominator;
			}

	std::array<double, N> coefficients{};
	coefficients[0] = dividedDifferences[N - 1];
	int degree = 0;
	for (int order = N - 2; order >= 0; order--)
		{
			std::array<double, N> expanded{};
			for (int power = 0; power <= degree; power++)
				{
					expanded[power] += -xa[order] * coefficients[power];
					expanded[power + 1] += coefficients[power];
				}
			expanded[0] += dividedDifferences[order];
			coefficients = expanded;
			degree++;
		}
	return coefficients;
}

void NNPDFDriver::lh_polint_coefficients_batch(
	const double xa[],
	const FlavorGridValues& yall,
	int row,
	FlavorInterpolationCoefficients& heldCoefficients)
{
	std::array<std::array<double, fN>, fN> inverseDenominators{};
	for (int order = 1; order < fN; order++)
		for (int i = order; i < fN; i++)
			{
				const double denominator = xa[i] - xa[i - order];
				if (denominator == 0.0) [[unlikely]]
					{
						cout << "failure in polint" << endl;
						exit(4);
					}
				inverseDenominators[order][i] = 1.0 / denominator;
			}

	for (std::size_t flavor = 0; flavor < fFlavorCount; flavor++)
		{
			auto& dividedDifferences = heldCoefficients[flavor].q2[row];
			dividedDifferences = yall[flavor][row];
			for (int order = 1; order < fN; order++)
				for (int i = fN - 1; i >= order; i--)
					dividedDifferences[i] =
						(dividedDifferences[i] - dividedDifferences[i - 1])
						* inverseDenominators[order][i];

			std::array<double, fN> coefficients{};
			coefficients[0] = dividedDifferences[fN - 1];
			int degree = 0;
			for (int order = fN - 2; order >= 0; order--)
				{
					std::array<double, fN> expanded{};
					for (int power = 0; power <= degree; power++)
					{
						expanded[power] += -xa[order] * coefficients[power];
						expanded[power + 1] += coefficients[power];
					}
					expanded[0] += dividedDifferences[order];
					coefficients = expanded;
					degree++;
				}
			dividedDifferences = coefficients;
		}
}


/////// Minimalist fortran wrapper
NNPDFDriver *pdf = NULL;
#ifdef __cplusplus
extern"C" {
#endif

  void initnnset_(const char* setname)
  {
    if (pdf) delete pdf;
    pdf = new NNPDFDriver(setname);
  }
  
  void initpdf_(int *mem)
  {
    if (!pdf) { cout << "initpdf: pdf not allocated" << endl; exit(-1); }
    pdf->initPDF(*mem);
  }

  double nnxfx_(double *x, double *Q, int *id)
  {
    if (!pdf) { cout << "nnxfx: pdf not allocated" << endl; exit(-1); }
    return pdf->xfx(*x,*Q,*id);
  }
  
#ifdef __cplusplus
}
#endif
