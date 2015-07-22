/*!
 * \file solver.cpp
 * \brief Class to store all solution data & apply FR operators
 *
 * \author - Jacob Crabill
 *           Aerospace Computing Laboratory (ACL)
 *           Aero/Astro Department. Stanford University
 *
 * \version 0.0.1
 *
 * Flux Reconstruction in C++ (Flurry++) Code
 * Copyright (C) 2014 Jacob Crabill.
 *
 */

#include "../include/solver.hpp"

#include <sstream>
#include <omp.h>

class intFace;
class boundFace;

#include "../include/input.hpp"
#include "../include/geo.hpp"
#include "../include/intFace.hpp"
#include "../include/boundFace.hpp"

solver::solver()
{
}

void solver::setup(input *params, geo *Geo)
{
  this->params = params;
  this->Geo = Geo;

  params->time = 0.;

  if (params->equation == NAVIER_STOKES) {
    if (params->nDims == 2)
      params->nFields = 4;
    else if (params->nDims == 3)
      params->nFields = 5;
  }

  /* Setup the FR elements & faces which will be computed on */
  Geo->setupElesFaces(eles,faces,mpiFaces);

  if (params->restart)
    readRestartFile();
  else
    setupElesFaces();

  /* Setup the FR operators for computation */
  setupOperators();

  /* Additional Setup */

  // Time advancement setup
  switch (params->timeType) {
    case 0:
      nRKSteps = 1;
      RKb = {1};
      break;
    case 4:
      nRKSteps = 4;
      RKa = {.5, .5, 1.};
      RKb = {1./6., 1./3., 1./3., 1./6.};
      break;
    default:
      FatalError("Time-Stepping type not supported.");
  }

#ifndef _NO_MPI
  finishMpiSetup();
#endif
}

void solver::update(void)
{
  // For RK time-stepping, store the starting solution values
  if (nRKSteps>1)
    copyUspts_U0();

  /* Intermediate residuals for Runge-Kutta time integration */

  for (int step=0; step<nRKSteps-1; step++) {

    if (step == 0)
      params->rkTime = params->time;
    else
      params->rkTime = params->time + RKa[step-1]*params->dt;

    moveMesh(step);

    calcResidual(step);

    /* If in first stage, compute CFL-based timestep */
    if (step == 0 && params->dtType == 1) calcDt();  // -- NOT CONSISTENT WITH MOVING MESH SEQUENCE --

    timeStepA(step);

  }

  /* Final Runge-Kutta time advancement step */

  if (nRKSteps == 1) {
    params->rkTime = params->time;
    /* Calculate CFL-based timestep */
    if (params->dtType == 1) calcDt();
  }
  else {
    params->rkTime = params->time + params->dt;
  }

  moveMesh(nRKSteps-1);

  calcResidual(nRKSteps-1);

  // Reset solution to initial-stage values
  if (nRKSteps>1)
    copyU0_Uspts();

  for (int step=0; step<nRKSteps; step++)
    timeStepB(step);

  params->time += params->dt;
}


void solver::calcResidual(int step)
{
  if(params->scFlag == 1) {
    shockCapture();
  }

  extrapolateU();

  /* --- Polynomial-Squeezing stabilization procedure --- */
  if (params->squeeze) {

    calcAvgSolution();

    checkEntropy();

  }

#ifndef _NO_MPI
  doCommunication();
#endif

  if (params->viscous || params->motion) {

    calcGradU_spts();

  }

  calcInviscidFlux_spts();

  calcInviscidFlux_faces();

#ifndef _NO_MPI
  calcInviscidFlux_mpi();
#endif

  if (params->viscous) {

    correctGradU();

    extrapolateGradU();

    calcViscousFlux_spts();

    calcViscousFlux_faces();

#ifndef _NO_MPI
    calcViscousFlux_mpi();
#endif

  }

  extrapolateNormalFlux();

  calcFluxDivergence(step);

  correctDivFlux(step);
}

void solver::calcDt(void)
{
  double dt = INFINITY;

#pragma omp parallel for reduction(min:dt)
  for (uint i=0; i<eles.size(); i++) {
    dt = min(dt, eles[i].calcDt());
  }

#ifndef _NO_MPI
  double dtTmp = dt;
  MPI_Allreduce(&dtTmp, &dt, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
#endif

  params->dt = dt;
}


void solver::timeStepA(int step)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].timeStepA(step,RKa[step]);
  }
}


void solver::timeStepB(int step)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].timeStepB(step,RKb[step]);
  }
}

void solver::copyUspts_U0(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].copyUspts_U0();
  }
}

void solver::copyU0_Uspts(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].copyU0_Uspts();
  }
}

void solver::extrapolateU(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    opers[eles[i].eType][eles[i].order].applySptsFpts(eles[i].U_spts,eles[i].U_fpts);
  }
}

void solver::calcAvgSolution()
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    opers[eles[i].eType][eles[i].order].calcAvgU(eles[i].U_spts,eles[i].detJac_spts,eles[i].Uavg);
  }
}

bool solver::checkDensity()
{
  bool squeezed = false;
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    bool check = eles[i].checkDensity();
    squeezed = check|| squeezed;
  }

  return squeezed;
}

void solver::checkEntropy()
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].checkEntropy();
  }
}

void solver::checkEntropyPlot()
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].checkEntropyPlot();
  }
}

void solver::extrapolateUMpts(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    opers[eles[i].eType][eles[i].order].applySptsMpts(eles[i].U_spts,eles[i].U_mpts);
  }
}

void solver::extrapolateSMpts(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    opers[eles[i].eType][eles[i].order].applySptsMpts(eles[i].S_spts,eles[i].S_mpts);
  }
}

void solver::extrapolateSFpts(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    opers[eles[i].eType][eles[i].order].applySptsFpts(eles[i].S_spts,eles[i].S_fpts);
  }
}

void solver::calcInviscidFlux_spts(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].calcInviscidFlux_spts();
  }
}

void solver::doCommunication()
{
#pragma omp parallel for
  for (uint i=0; i<mpiFaces.size(); i++) {
    mpiFaces[i]->communicate();
  }
}

void solver::calcInviscidFlux_faces()
{
#pragma omp parallel for
  for (uint i=0; i<faces.size(); i++) {
    faces[i]->calcInviscidFlux();
  }
}

void solver::calcInviscidFlux_mpi()
{
#pragma omp parallel for
  for (uint i=0; i<mpiFaces.size(); i++) {
    mpiFaces[i]->calcInviscidFlux();
  }
}

void solver::calcViscousFlux_spts(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].calcViscousFlux_spts();
  }
}

void solver::calcViscousFlux_faces()
{
#pragma omp parallel for
  for (uint i=0; i<faces.size(); i++) {
    faces[i]->calcViscousFlux();
  }
}


void solver::calcViscousFlux_mpi()
{
#pragma omp parallel for
  for (uint i=0; i<mpiFaces.size(); i++) {
    mpiFaces[i]->calcViscousFlux();
  }
}


void solver::calcGradF_spts(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    opers[eles[i].eType][eles[i].order].applyGradFSpts(eles[i].F_spts,eles[i].dF_spts);
  }
}

void solver::transformGradF_spts(int step)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].transformGradF_spts(step);
  }
}

void solver::calcFluxDivergence(int step)
{
  if (params->motion) {

    /* Use non-conservation-form chain-rule transformation
     * (See AIAA paper 2013-0998 by Liang, Miyaji and Zhang) */

    calcGradF_spts();

    transformGradF_spts(step);

  }else{

    /* Standard conservative form */
    calcDivF_spts(step);

  }
}

void solver::calcDivF_spts(int step)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    opers[eles[i].eType][eles[i].order].applyDivFSpts(eles[i].F_spts,eles[i].divF_spts[step]);
  }
}

void solver::extrapolateNormalFlux(void)
{
  if (params->motion) {
    /* Extrapolate physical normal flux */
#pragma omp parallel for
    for (uint i=0; i<eles.size(); i++) {
      opers[eles[i].eType][eles[i].order].applyExtrapolateFn(eles[i].F_spts,eles[i].norm_fpts,eles[i].disFn_fpts,eles[i].dA_fpts);
    }
  }
  else {
    /* Extrapolate transformed normal flux */
#pragma omp parallel for
    for (uint i=0; i<eles.size(); i++) {
      opers[eles[i].eType][eles[i].order].applyExtrapolateFn(eles[i].F_spts,eles[i].tNorm_fpts,eles[i].disFn_fpts);
    }
  }
}

void solver::correctDivFlux(int step)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].calcDeltaFn();
    opers[eles[i].eType][eles[i].order].applyCorrectDivF(eles[i].dFn_fpts,eles[i].divF_spts[step]);
  }
}

void solver::calcGradU_spts(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    opers[eles[i].eType][eles[i].order].applyGradSpts(eles[i].U_spts,eles[i].dU_spts);
  }
}

void solver::correctGradU(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].calcDeltaUc();
    opers[eles[i].eType][eles[i].order].applyCorrectGradU(eles[i].dUc_fpts,eles[i].dU_spts);
  }
}

void solver::extrapolateGradU()
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    for (int dim=0; dim<params->nDims; dim++) {
      opers[eles[i].eType][eles[i].order].applySptsFpts(eles[i].dU_spts[dim],eles[i].dU_fpts[dim]);
    }
  }
}

void solver::calcEntropyErr_spts(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].calcEntropyErr_spts();
  }
}

void solver::moveMesh(int step)
{
  if (!params->motion) return;

#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].move(step);
  }
}

vector<double> solver::computeWallForce(void)
{
  vector<double> force(params->nDims);

  for (uint i=0; i<faces.size(); i++) {
    auto fTmp = faces[i]->computeWallForce();

    for (int dim=0; dim<params->nDims; dim++)
      force[dim] += fTmp[dim];
  }

  return force;
}

void solver::setupOperators()
{
  if (params->rank==0) cout << "Solver: Setting up FR operators" << endl;

  // Get all element types & olynomial orders in mesh
  for (auto& e:eles) {
    eTypes.insert(e.eType);
    polyOrders[e.eType].insert(e.order);
  }

  for (auto& e: eTypes) {
    for (auto& p: polyOrders[e]) {
      opers[e][p].setupOperators(e,p,Geo,params);
    }
  }
}

void solver::setupElesFaces(void) {

  if (params->rank==0) cout << "Solver: Setting up elements & faces" << endl;

#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].setup(params,Geo);
  }

  // Finish setting up internal & boundary faces
#pragma omp parallel for
  for (uint i=0; i<faces.size(); i++) {
    faces[i]->setupFace();
  }

  // Finish setting up MPI faces
#pragma omp parallel for
  for (uint i=0; i<mpiFaces.size(); i++) {
    mpiFaces[i]->setupFace();
  }
}

void solver::finishMpiSetup(void)
{
  if (params->rank==0) cout << "Solver: Setting up MPI face communicataions" << endl;
#pragma omp parallel for
  for (uint i=0; i<mpiFaces.size(); i++) {
    mpiFaces[i]->finishRightSetup();
  }
}

void solver::readRestartFile(void) {

  ifstream dataFile;
  dataFile.precision(15);

  // Get the file name & open the file
  char fileNameC[256];
  string fileName = params->dataFileName;
#ifndef _NO_MPI
  /* --- All processors write their solution to their own .vtu file --- */
  sprintf(fileNameC,"%s_%.09d/%s_%.09d_%d.vtu",&fileName[0],params->restartIter,&fileName[0],params->restartIter,params->rank);
#else
  sprintf(fileNameC,"%s_%.09d.vtu",&fileName[0],params->restartIter);
#endif

  if (params->rank==0) cout << "Solver: Restarting from " << fileNameC << endl;

  dataFile.open(fileNameC);

  if (!dataFile.is_open())
    FatalError("Cannont open restart file.");

  // Find the start of the UnstructuredData region
  bool found = false;
  string str;
  while (getline(dataFile,str)) {
    stringstream ss;
    ss.str(str);
    ss >> str;
    if (str.compare("<UnstructuredGrid>")==0) {
      found = true;
      break;
    }
  }

  if (!found)
    FatalError("Cannot find UnstructuredData tag in restart file.");

  // Read restart data & setup all data arrays
  for (auto& e:eles) {
    e.restart(dataFile,params,Geo);
  }

  dataFile.close();

  if (params->rank==0) cout << "Solver: Done reading restart file." << endl;

  // Setup all transformations and other geometry-related arrays
  if (params->rank==0) cout << "Ele: Setting up all element geometry data." << endl;
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].setupAllGeometry();
  }

  // Finish setting up internal faces
  if (params->rank==0) cout << "Face: Setting up all face data." << endl;
#pragma omp parallel for
  for (uint i=0; i<faces.size(); i++) {
    faces[i]->setupFace();
  }

  // Finish setting up MPI faces
  if (params->rank==0 && params->nproc>1) cout << "MPIFace: Setting up MPI face communications." << endl;
#pragma omp parallel for
  for (uint i=0; i<mpiFaces.size(); i++) {
    mpiFaces[i]->setupFace();
  }
}

void solver::initializeSolution()
{
  if (params->rank==0) cout << "Solver: Initializing Solution... " << flush;

#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].setInitialCondition();
  }

  /* If running a moving-mesh case and using CFL-based time-stepping,
   * calc initial dt for grid velocity calculation */
  /* If running a moving-mesh case and using CFL-based time-stepping,
   * calc initial dt for grid velocity calculation */
  //if ( (params->motion!=0 || params->slipPenalty==1) && params->dtType == 1 ) {
  if (params->dtType == 1) {
    extrapolateU();
    double dt = INFINITY;
#pragma omp parallel for reduction(min:dt)
    for (uint i=0; i<eles.size(); i++) {
      eles[i].calcWaveSpFpts();
      dt = std::min(dt, eles[i].calcDt());
    }

#ifndef _NO_MPI
  double dtTmp = dt;
  MPI_Allreduce(&dtTmp, &dt, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
#endif

    params->dt = dt;
  }

  if (params->rank == 0) cout << "done." << endl;
}

// Method for shock capturing
void solver::shockCapture(void)
{
#pragma omp parallel for
  for (uint i=0; i<eles.size(); i++) {
    eles[i].sensor = opers[eles[i].eType][eles[i].order].shockCaptureInEle(eles[i].U_spts,params->threshold);
  }
}
