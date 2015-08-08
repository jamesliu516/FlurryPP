/*!
 * \file funcs.cpp
 * \brief Miscellaneous helper functions
 *
 * \author - Jacob Crabill
 *           Aerospace Computing Laboratory (ACL)
 *           Aero/Astro Department. Stanford University
 *
 * \version 0.0.1
 *
 * Flux Reconstruction in C++ (Flurry++) Code
 * Copyright (C) 2015 Jacob Crabill.
 *
 */
#include "funcs.hpp"

void shape_quad(const point &in_rs, vector<double> &out_shape, int nNodes)
{
  out_shape.resize(nNodes);
  shape_quad(in_rs, out_shape.data(), nNodes);
}

void shape_quad(const point &in_rs, double* out_shape, int nNodes)
{
  double xi  = in_rs.x;
  double eta = in_rs.y;
  switch(nNodes) {
  case 4:
    out_shape[0] = 0.25*(1-xi)*(1-eta);
    out_shape[1] = 0.25*(1+xi)*(1-eta);
    out_shape[2] = 0.25*(1+xi)*(1+eta);
    out_shape[3] = 0.25*(1-xi)*(1+eta);
    break;

  case 8:
    out_shape[0] = -0.25*(1-xi)*(1-eta)*(1+eta+xi);
    out_shape[1] = -0.25*(1+xi)*(1-eta)*(1+eta-xi);
    out_shape[2] = -0.25*(1+xi)*(1+eta)*(1-eta-xi);
    out_shape[3] = -0.25*(1-xi)*(1+eta)*(1-eta+xi);
    out_shape[4] = 0.5*(1-xi)*(1+ xi)*(1-eta);
    out_shape[5] = 0.5*(1+xi)*(1+eta)*(1-eta);
    out_shape[6] = 0.5*(1-xi)*(1+ xi)*(1+eta);
    out_shape[7] = 0.5*(1-xi)*(1+eta)*(1-eta);
    break;
  }
}

void shape_hex(const point &in_rst, vector<double> &out_shape, int nNodes)
{
  out_shape.resize(nNodes);
  shape_hex(in_rst, out_shape.data(), nNodes);
}

void shape_hex(const point &in_rst, double* out_shape, int nNodes)
{
  double xi  = in_rst.x;
  double eta = in_rst.y;
  double mu = in_rst.z;
  switch(nNodes) {
    case 8:
      out_shape[0] = 0.125*(1-xi)*(1-eta)*(1-mu);
      out_shape[1] = 0.125*(1+xi)*(1-eta)*(1-mu);
      out_shape[2] = 0.125*(1+xi)*(1+eta)*(1-mu);
      out_shape[3] = 0.125*(1-xi)*(1+eta)*(1-mu);

      out_shape[4] = 0.125*(1-xi)*(1-eta)*(1+mu);
      out_shape[5] = 0.125*(1+xi)*(1-eta)*(1+mu);
      out_shape[6] = 0.125*(1+xi)*(1+eta)*(1+mu);
      out_shape[7] = 0.125*(1-xi)*(1+eta)*(1+mu);
      break;

    case 20: {
      double XI[8]  = {-1,1,1,-1,-1,1,1,-1};
      double ETA[8] = {-1,-1,1,1,-1,-1,1,1};
      double MU[8]  = {-1,-1,-1,-1,1,1,1,1};
      // Corner nodes
      for (int i=0; i<8; i++) {
        out_shape[i] = .125*(1+xi*XI[i])*(1+eta*ETA[i])*(1+mu*MU[i])*(xi*XI[i]+eta*ETA[i]+mu*MU[i]-2);
      }
      // Edge nodes, xi = 0
      out_shape[8]  = .25*(1-xi*xi)*(1-eta)*(1-mu);
      out_shape[10] = .25*(1-xi*xi)*(1+eta)*(1-mu);
      out_shape[16] = .25*(1-xi*xi)*(1-eta)*(1+mu);
      out_shape[18] = .25*(1-xi*xi)*(1+eta)*(1+mu);
      // Edge nodes, eta = 0
      out_shape[9]  = .25*(1-eta*eta)*(1+xi)*(1-mu);
      out_shape[11] = .25*(1-eta*eta)*(1-xi)*(1-mu);
      out_shape[17] = .25*(1-eta*eta)*(1+xi)*(1+mu);
      out_shape[19] = .25*(1-eta*eta)*(1-xi)*(1+mu);
      // Edge Nodes, mu = 0
      out_shape[12] = .25*(1-mu*mu)*(1-xi)*(1-eta);
      out_shape[13] = .25*(1-mu*mu)*(1+xi)*(1-eta);
      out_shape[14] = .25*(1-mu*mu)*(1+xi)*(1+eta);
      out_shape[15] = .25*(1-mu*mu)*(1-xi)*(1+eta);
      break;
    }
  }
}

void dshape_quad(const point &in_rs, matrix<double> &out_dshape, int nNodes)
{
  double xi  = in_rs.x;
  double eta = in_rs.y;
  out_dshape.setup(nNodes,2);

  switch(nNodes) {
  case 4:
    out_dshape(0,0) = -0.25*(1-eta);
    out_dshape(1,0) =  0.25*(1-eta);
    out_dshape(2,0) =  0.25*(1+eta);
    out_dshape(3,0) = -0.25*(1+eta);

    out_dshape(0,1) = -0.25*(1-xi);
    out_dshape(1,1) = -0.25*(1+xi);
    out_dshape(2,1) =  0.25*(1+xi);
    out_dshape(3,1) =  0.25*(1-xi);
    break;

  case 8:
    out_dshape(0,0) = -0.25*(-1+eta)*(2*xi+eta);
    out_dshape(1,0) =  0.25*(-1+eta)*(eta - 2*xi);
    out_dshape(2,0) =  0.25*( 1+eta)*(2*xi+eta);
    out_dshape(3,0) = -0.25*( 1+eta)*(eta-2*xi);
    out_dshape(4,0) =    xi*(-1+eta);
    out_dshape(5,0) = -0.5 *( 1+eta)*(-1+eta);
    out_dshape(6,0) =   -xi*( 1+eta);
    out_dshape(7,0) =  0.5 *( 1+eta)*(-1+eta);

    out_dshape(0,1) = -0.25*(-1+xi)*(2*eta+xi);
    out_dshape(1,1) =  0.25*( 1+xi)*(2*eta - xi);
    out_dshape(2,1) =  0.25*( 1+xi)*(2*eta+xi);
    out_dshape(3,1) = -0.25*(-1+xi)*(2*eta-xi);
    out_dshape(4,1) =  0.5 *( 1+xi)*(-1+xi);
    out_dshape(5,1) =  -eta*( 1+xi);
    out_dshape(6,1) = -0.5 *( 1+xi)*(-1+xi);
    out_dshape(7,1) =   eta*(-1+xi);
    break;
  }
}

void dshape_hex(const point &in_rst, matrix<double> &out_dshape, int nNodes)
{
  double xi  = in_rst.x;
  double eta = in_rst.y;
  double mu = in_rst.z;
  out_dshape.setup(nNodes,3);

  switch(nNodes) {
    case 8:
      out_dshape(0,0) = -0.125*(1-eta)*(1-mu);
      out_dshape(1,0) =  0.125*(1-eta)*(1-mu);
      out_dshape(2,0) =  0.125*(1+eta)*(1-mu);
      out_dshape(3,0) = -0.125*(1+eta)*(1-mu);

      out_dshape(4,0) = -0.125*(1-eta)*(1+mu);
      out_dshape(5,0) =  0.125*(1-eta)*(1+mu);
      out_dshape(6,0) =  0.125*(1+eta)*(1+mu);
      out_dshape(7,0) = -0.125*(1+eta)*(1+mu);

      out_dshape(0,1) = -0.125*(1-xi)*(1-mu);
      out_dshape(1,1) = -0.125*(1+xi)*(1-mu);
      out_dshape(2,1) =  0.125*(1+xi)*(1-mu);
      out_dshape(3,1) =  0.125*(1-xi)*(1-mu);

      out_dshape(4,1) = -0.125*(1-xi)*(1+mu);
      out_dshape(5,1) = -0.125*(1+xi)*(1+mu);
      out_dshape(6,1) =  0.125*(1+xi)*(1+mu);
      out_dshape(7,1) =  0.125*(1-xi)*(1+mu);

      out_dshape(0,2) = -0.125*(1-xi)*(1-eta);
      out_dshape(1,2) = -0.125*(1+xi)*(1-eta);
      out_dshape(2,2) = -0.125*(1+xi)*(1+eta);
      out_dshape(3,2) = -0.125*(1-xi)*(1+eta);

      out_dshape(4,2) =  0.125*(1-xi)*(1-eta);
      out_dshape(5,2) =  0.125*(1+xi)*(1-eta);
      out_dshape(6,2) =  0.125*(1+xi)*(1+eta);
      out_dshape(7,2) =  0.125*(1-xi)*(1+eta);
      break;
    case 20: {
      double XI[8]  = {-1,1,1,-1,-1,1,1,-1};
      double ETA[8] = {-1,-1,1,1,-1,-1,1,1};
      double MU[8]  = {-1,-1,-1,-1,1,1,1,1};
      // Corner Nodes
      for (int i=0; i<8; i++) {
        out_dshape(i,0) = .125*XI[i] *(1+eta*ETA[i])*(1 + mu*MU[i])*(2*xi*XI[i] +   eta*ETA[i] +   mu*MU[i]-1);
        out_dshape(i,1) = .125*ETA[i]*(1 + xi*XI[i])*(1 + mu*MU[i])*(  xi*XI[i] + 2*eta*ETA[i] +   mu*MU[i]-1);
        out_dshape(i,2) = .125*MU[i] *(1 + xi*XI[i])*(1+eta*ETA[i])*(  xi*XI[i] +   eta*ETA[i] + 2*mu*MU[i]-1);
      }
      // Edge Nodes, xi = 0
      out_dshape( 8,0) = -.5*xi*(1-eta)*(1-mu);  out_dshape( 8,1) = -.25*(1-xi*xi)*(1-mu);  out_dshape( 8,2) = -.25*(1-xi*xi)*(1-eta);
      out_dshape(10,0) = -.5*xi*(1+eta)*(1-mu);  out_dshape(10,1) =  .25*(1-xi*xi)*(1-mu);  out_dshape(10,2) = -.25*(1-xi*xi)*(1+eta);
      out_dshape(16,0) = -.5*xi*(1-eta)*(1+mu);  out_dshape(16,1) = -.25*(1-xi*xi)*(1+mu);  out_dshape(16,2) =  .25*(1-xi*xi)*(1-eta);
      out_dshape(18,0) = -.5*xi*(1+eta)*(1+mu);  out_dshape(18,1) =  .25*(1-xi*xi)*(1+mu);  out_dshape(18,2) =  .25*(1-xi*xi)*(1+eta);
      // Edge Nodes, eta = 0
      out_dshape( 9,1) = -.5*eta*(1+xi)*(1-mu);  out_dshape( 9,0) =  .25*(1-eta*eta)*(1-mu);  out_dshape( 9,2) = -.25*(1-eta*eta)*(1+xi);
      out_dshape(11,1) = -.5*eta*(1-xi)*(1-mu);  out_dshape(11,0) = -.25*(1-eta*eta)*(1-mu);  out_dshape(11,2) = -.25*(1-eta*eta)*(1-xi);
      out_dshape(17,1) = -.5*eta*(1+xi)*(1+mu);  out_dshape(17,0) =  .25*(1-eta*eta)*(1+mu);  out_dshape(17,2) =  .25*(1-eta*eta)*(1+xi);
      out_dshape(19,1) = -.5*eta*(1-xi)*(1+mu);  out_dshape(19,0) = -.25*(1-eta*eta)*(1+mu);  out_dshape(19,2) =  .25*(1-eta*eta)*(1-xi);
      // Edge Nodes, mu = 0;
      out_dshape(12,2) = -.5*mu*(1-xi)*(1-eta);  out_dshape(12,0) = -.25*(1-mu*mu)*(1-eta);  out_dshape(12,1) = -.25*(1-mu*mu)*(1-xi);
      out_dshape(13,2) = -.5*mu*(1+xi)*(1-eta);  out_dshape(13,0) =  .25*(1-mu*mu)*(1-eta);  out_dshape(13,1) = -.25*(1-mu*mu)*(1+xi);
      out_dshape(14,2) = -.5*mu*(1+xi)*(1+eta);  out_dshape(14,0) =  .25*(1-mu*mu)*(1+eta);  out_dshape(14,1) =  .25*(1-mu*mu)*(1+xi);
      out_dshape(15,2) = -.5*mu*(1-xi)*(1+eta);  out_dshape(15,0) = -.25*(1-mu*mu)*(1+eta);  out_dshape(15,1) =  .25*(1-mu*mu)*(1-xi);
      break;
    }
  }
}

void shape_tri(const point &in_rs, vector<double> &out_shape)
{
  // For the shape function for a general N-noded triangle, refer
  // to Finite Element Methods by Hughes, p. 166
  out_shape.resize(3); // nNodes

  out_shape[0] = in_rs.x;
  out_shape[1] = in_rs.y;
  out_shape[2] = 1 - in_rs.x - in_rs.y;
}

void shape_tri(const point &in_rs, double* out_shape)
{
  out_shape[0] = in_rs.x;
  out_shape[1] = in_rs.y;
  out_shape[2] = 1 - in_rs.x - in_rs.y;
}

void dshape_tri(point &, matrix<double> &out_dshape)
{
  out_dshape.setup(3,2); // nNodes, nDims

  out_dshape(0,0) =  1;
  out_dshape(1,0) =  0;
  out_dshape(2,0) = -1;

  out_dshape(0,1) =  0;
  out_dshape(1,1) =  1;
  out_dshape(2,1) = -1;
}

void shape_tet(const point &in_rs, vector<double> &out_shape)
{
  out_shape.resize(4); // nNodes
  shape_tet(in_rs, out_shape.data());
}

void shape_tet(const point &in_rs, double* out_shape)
{
  out_shape[0] = in_rs.x;
  out_shape[1] = in_rs.y;
  out_shape[2] = in_rs.z;
  out_shape[3] = 1 - in_rs.x - in_rs.y - in_rs.z;
}

void dshape_tet(point &, matrix<double> &out_dshape)
{
  out_dshape.setup(4,3); // nNodes, nDims

  out_dshape(0,0) =  1;
  out_dshape(1,0) =  0;
  out_dshape(2,0) =  0;
  out_dshape(3,0) = -1;

  out_dshape(0,1) =  0;
  out_dshape(1,1) =  1;
  out_dshape(2,1) =  0;
  out_dshape(3,1) = -1;

  out_dshape(0,2) =  0;
  out_dshape(1,2) =  0;
  out_dshape(2,2) =  1;
  out_dshape(3,2) = -1;
}

void getSimplex(int nDims, vector<double> x0, double L, matrix<double> X)
{
//  vector<double> dx(nDims+1,0);
//  X.setup(nDims,nDims+1);
//  X.initializeToZero();

//  for (int i=0; i<nDims; i++) {
//    dx[i+1] = sqrt(1-(i*dx[i]/(i+1))^2);
//    vector<int> ei(nDims); ei[i-1] = 1;
//    for (int j=0; j<nDims+1; j++) {
//      X(j,i+1) = 1/(i)*sum(X,2) + dx[i]*ei[j];
//    }
//  }

//  X *= L;
//  for (int i=0; i<nDims; i++) {
//    for (int j=0; j<nDims+1; j++) {
//      X(i,j) += x0[i];
//    }
//  }
}

vector<int> getOrder(vector<double> &data)
{
  vector<pair<double,size_t> > vp;
  vp.reserve(data.size());
  for (size_t i = 0 ; i != data.size() ; i++) {
    vp.push_back(make_pair(data[i], i));
  }

  // Sorting will put lower values [vp.first] ahead of larger ones,
  // resolving ties using the original index [vp.second]
  sort(vp.begin(), vp.end());
  vector<int> ind(data.size());
  for (size_t i = 0 ; i != vp.size() ; i++) {
    ind[i] = vp[i].second;
  }

  return ind;
}
