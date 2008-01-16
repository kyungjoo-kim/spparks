/* ----------------------------------------------------------------------
   SPPARKS - Stochastic Parallel PARticle Kinetic Simulator
   contact info, copyright info, etc
 ------------------------------------------------------------------------- */

#include "math.h"
#include "mpi.h"
#include "string.h"
#include "stdlib.h"
#include "app_potts_variable.h"
#include "comm_lattice.h"
#include "solve.h"
#include "random_park.h"
#include "timer.h"
#include "memory.h"
#include "error.h"

#include <map>

using namespace SPPARKS;

/* ---------------------------------------------------------------------- */

AppPottsVariable::AppPottsVariable(SPK *spk, int narg, char **arg) : 
  AppLattice(spk,narg,arg)
{
  // parse arguments

  if (narg < 3) error->all("Invalid app_style potts/variable command");

  seed = atoi(arg[1]);
  nspins = atoi(arg[2]);

  random = new RandomPark(seed);

  options(narg-3,&arg[3]);

  // define lattice and partition it across processors
  
  create_lattice();
  sites = new int[1 + maxneigh];

  // assign variable names

  if (ninteger != 1 || ndouble != 0.0)
    error->all("Invalid site specification in app_style potts/variable");

  spin = iarray[0];

  // initialize my portion of lattice
  // each site = one of nspins
  // loop over global list so assignment is independent of # of procs
  // use map to see if I own global site

  std::map<int,int> hash;
  for (int i = 0; i < nlocal; i++)
    hash.insert(std::pair<int,int> (id[i],i));
  std::map<int,int>::iterator loc;

  int ilocal,isite;
  for (int iglobal = 1; iglobal <= nglobal; iglobal++) {
    isite = random->irandom(nspins);
    loc = hash.find(iglobal);
    if (loc != hash.end()) spin[loc->second] = isite;
  }
}

/* ---------------------------------------------------------------------- */

AppPottsVariable::~AppPottsVariable()
{
  delete random;
  delete [] sites;
  delete comm;
}

/* ----------------------------------------------------------------------
   compute energy of site
------------------------------------------------------------------------- */

double AppPottsVariable::site_energy(int i)
{
  int isite = spin[i];
  int eng = 0;
  for (int j = 0; j < numneigh[i]; j++)
    if (isite != spin[neighbor[i][j]]) eng++;
  return (double) eng;
}

/* ----------------------------------------------------------------------
   randomly pick new state for site
------------------------------------------------------------------------- */

void AppPottsVariable::site_pick_random(int i, double ran)
{
  int iran = (int) (nspins*ran) + 1;
  if (iran > nspins) iran = nspins;
  spin[i] = iran;
}

/* ----------------------------------------------------------------------
   randomly pick new state for site from neighbor values
------------------------------------------------------------------------- */

void AppPottsVariable::site_pick_local(int i, double ran)
{
  int iran = (int) (numneigh[i]*ran) + 1;
  if (iran > numneigh[i]) iran = numneigh[i];
  spin[i] = spin[neighbor[i][iran]];
}

/* ----------------------------------------------------------------------
   compute total propensity of owned site
   based on einitial,efinal for each possible event
   if no energy change, propensity = 1
   if downhill energy change, propensity = 1
   if uphill energy change, propensity set via Boltzmann factor
   if proc owns full domain, there are no ghosts, so ignore full flag
------------------------------------------------------------------------- */

double AppPottsVariable::site_propensity(int i, int full)
{
  int j,k,value;
  double efinal;

  // possible events = spin flips to neighboring site different than self

  int nevent = 0;
  for (j = 0; j < numneigh[i]; j++) {
    value = spin[neighbor[i][j]];
    if (value == spin[i]) continue;
    for (k = 0; k < nevent; k++)
      if (value == sites[k]) break;
    if (k < nevent) continue;
    sites[nevent++] = value;
  }

  // for each possible flip:
  // compute energy difference between initial and final state
  // sum to prob for all events on this site

  int oldstate = spin[i];
  double einitial = site_energy(i);
  double prob = 0.0;

  for (k = 0; k < nevent; k++) {
    spin[i] = sites[k];
    efinal = site_energy(i);
    if (efinal <= einitial) prob += 1.0;
    else if (temperature > 0.0) prob += exp((einitial-efinal)*t_inverse);
  }

  spin[i] = oldstate;
  return prob;
}

/* ----------------------------------------------------------------------
   choose and perform an event for site
   update propensities of all affected sites
   if proc owns full domain, there are no ghosts, so ignore full flag
   if proc owns sector, ignore neighbor sites that are ghosts
------------------------------------------------------------------------- */

void AppPottsVariable::site_event(int i, int full)
{
  int j,k,m,isite,value;
  double efinal;

  // pick one event from total propensity

  double threshhold = random->uniform() * propensity[i2site[i]];

  // possible events = spin flips to neighboring site different than self
  // find one event, accumulate its probability
  // compare prob to threshhold, break when reach it to select event

  double einitial = site_energy(i);
  double prob = 0.0;
  int nevent = 0;

  for (j = 0; j < numneigh[i]; j++) {
    value = spin[neighbor[i][j]];
    if (value == spin[i]) continue;
    for (k = 0; k < nevent; k++)
      if (value == sites[k]) break;
    if (k < nevent) continue;
    sites[nevent++] = value;

    spin[i] = sites[k];
    efinal = site_energy(i);
    if (efinal <= einitial) prob += 1.0;
    else if (temperature > 0.0) prob += exp((einitial-efinal)*t_inverse);

    if (prob >= threshhold) break;
  }

  // compute propensity changes for self and neighbor sites

  int nsites = 0;
  isite = i2site[i];
  sites[nsites++] = isite;
  propensity[isite] = site_propensity(i,full);

  for (j = 0; j < numneigh[i]; j++) {
    m = neighbor[i][j];
    isite = i2site[m];
    if (isite < 0) continue;
    sites[nsites++] = isite;
    propensity[isite] = site_propensity(m,full);
  }

  solve->update(nsites,sites,propensity);
}

/* ----------------------------------------------------------------------
  clear mask values of site and its neighbors
  OK to clear ghost site mask values
------------------------------------------------------------------------- */

void AppPottsVariable::site_clear_mask(char *mask, int i)
{
  mask[i] = 0;
  for (int j = 0; j < numneigh[i]; j++) mask[neighbor[i][j]] = 0;
}