#ifndef MYFUNCTIONS_H_
#define MYFUNCTIONS_H_

#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_BCRec.H>

#include <LGFCore.H>

using namespace amrex;

void test_print();
void extendedMain();

void initializeSourceMultiFab(amrex::MultiFab& phifab, amrex::Geometry& geom);

#endif