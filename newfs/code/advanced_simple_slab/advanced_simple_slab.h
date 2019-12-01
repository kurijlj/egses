/*
###############################################################################
#
#  EGSnrc egs++ Advanced Simple Slab application
#  Copyright (C) 2015 National Research Council Canada
#
#  This file is part of EGSnrc.
#
#  EGSnrc is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Affero General Public License as published by the
#  Free Software Foundation, either version 3 of the License, or (at your
#  option) any later version.
#
#  EGSnrc is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
#  FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
#  more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with EGSnrc. If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################
#
#  Author:          Iwan Kawrakow, 2005
#
#  Contributors:    Frederic Tessier, Ljubomir Kurij
#
###############################################################################
#
#  A relatively simple EGSnrc application using the C++ interface. It is based
#  on the tutor7pp application with some functionality stripped off and some
#  new functionality added.
#
#  In addition, tutor7pp derives from the EGS_AdvancedApplication class and
#  therefore automatically inherits the ability to do restarted and parallel
#  simulations, to combine the results of parallel runs or to re-analyze the
#  results of single/parallel runs. It also inherits the ability to run for a
#  user specified maximum amount of cpu time or to terminate the simulation
#  when a user specified uncertainty has been reached.
#
#
#  TERMINOLOGY
#  -----------
#
#  Simulations are split into 'chunks'. For simple simulations (no parallel
#  runs, etc.) there is a single simulation chunk with the number of
#  histories specified in the input file. For parallel runs the number of
#  chunks and number of histories per chunk are determined by a 'run control
#  object' (see below).
#
#  Each simulation chunk is split into 'batches'. The batches are not required
#  for statistical analysis (by using the provided scoring classes it is easy
#  to have a history-by-history uncertainty estimation). Instead, simulation
#  chunks are split into batches so that the progress of the simulation can be
#  reported after the completion of a batch and the current results can be
#  stored into a data file. By default there are 10 batches per simulation chunk
#  but this can be changed in the input file.
#
#  The simulation is controlled via a 'run control object' (RCO) The purpose
#  of the run control object is to give to the shower loop the number of
#  histories per simulation chunk, number of batches per chunk and to possibly
#  terminate the simulation prematurely if certain conditions are met (e.g.
#  maximum cpu time allowed is exceeded or the required uncertainty has been
#  reached).
#
#  egs++ provides 2 run control objects:
#
#  1)  simple:  the simple RCO always uses a single simulation chunk.
#
#  2)  JCF:     a JCF object is used by default for parallel runs
#               JCF stands for Job Control File as this type of object
#               uses a file placed in the user code directory to record
#               the number of histories remaining, the number of jobs
#               running, etc., in parallel runs. This is explained in
#               more details in PIRS-877. A JCF object uses by default
#               10 simulation chunks but this can be changed in the
#               input file.
#
#  It is possible to use a simple control object for parallel runs by giving
#  the -s or --simple command line option. In this case, each parallel job
#  will run the number of histories specified in the input file but
#  automatically adjust the initial random number seed(s) with the job index.
#  This additional possibility has been implemented because several users have
#  reported problems with file locking needed for a JCF run control object.
#
#  It is also possible to have other RCO's compiled into shared libraries and
#  automatically loaded at run time (e.g., one could implement a RCO that
#  communicates via TCP/IP with a remote server to obtain the number of
#  histories in the next simulation chunk).
#
#
#  USAGE
#  -----
#
#  - Geometry and particle source are specified in an input file as explained
#    in PIRS-899 and PIRS-898.
#
#  - Run control is specified in a section of the input file delimited by
#    :start run control: and :stop run control: labels.
#
#  - A simple RCO is used for single job runs.
#
#  - A JCF RCO is used by default for parallel runs, unless -s or --simple
#    is specified on the command line.
#
#  - A simple RCO understands the following keys:
#    ncase                       = number of histories to run
#    nbatch                      = number of batches to use
#    statistical accuracy sought = required uncertainty, in %
#    max cpu hours allowed       = max. processor time allowed
#    calculation                 = first | restart | combine | analyze
#
#    All inputs except for ncase are optional (a missing ncase key will result
#    in a simulation with 0 particles).
#
#  - A JCF object understands all the above keys plus
#    nchunk = number of simulation chunks
#
#  - The simulation is run using
#
#    advanced_simple_slab -i input_file -p pegs_file
#                         [-o output_file] [-s] [-P n -j i]
#
#    where command line arguments between [] are optional. The -P n option
#    specifies the number of parallel jobs n and -j i the index of this job.
#    On Linux/Unix systems it is more convenient to use the 'exb' script for
#    parallel job submission (see PIRS-877)
#
###############################################################################
*/


#ifndef ADVANCED_SIMPLE_SLAB_
#define ADVANCED_SIMPLE_SLAB_

#include <array>                      // Required by array container class.
#include <vector>                     // Required by vector container class.
#include "egs_advanced_application.h" // We derive from EGS_AdvancedApplication.
#include "egs_scoring.h"              // Required by EGS_ScoringArray class.
#include "egs_input.h"                // Required by EGS_Input class.

using namespace std;

class APP_EXPORT AdvancedSimpleSlabCode : public EGS_AdvancedApplication {

    int              nreg;            // Number of regions in the geometry.
    int              rr_flag;         // Used for Russian Roulette and
                                      // radiative splitting.
    int              prev_case;       // Previous case index tracker.
    double           Etot;            // Total energy that has entered
                                      // the geometry.
    EGS_ScoringArray *score;          // Scoring deposited energies.
    EGS_ScoringArray *eflu;           // Scoring electron fluence at the back of
                                      // an geometry.
    EGS_ScoringArray *gflu;           // Scoring photon fluence at the back of
                                      // an geometry.
    EGS_Float        current_weight;  // The weight of the initial particle that
                                      // is currently being simulated.
    vector<int>      np_hist;         // Stack pointer history.
    array<int, 1024> np_map;          // Map of active stack pointer indexes.
    bool             deflect_brems;   // Deflect electron after brems
                                      // option flag.
    static string    revision;        // The CVS revision number.

public:

    /*! Constructor
     The command line arguments are passed to the EGS_AdvancedApplication
     contructor, which determines the input file, the pegs file, if the
     simulation is a parallel run, etc.
    */
    AdvancedSimpleSlabCode(int argc, char **argv) :
        EGS_AdvancedApplication(argc,argv), nreg(0), rr_flag(0), prev_case(0),
        Etot(0), score(0), eflu(0), gflu(0), current_weight(1),
        deflect_brems(false) { };

    /*! Destructor.
     Deallocate memory
     */
    ~AdvancedSimpleSlabCode() {
        if(score) delete score;
        if(eflu) delete eflu;
        if(gflu) delete gflu;
    };

    /*! Describe the application.
     This function is called from within the initSimulation() function so that
     applications derived from EGS_AdvancedApplication can print a header at
     the beginning of the output file.
    */
    void describeUserCode() const;

    /*! Initialize scoring.
     This function is called from within initSimulation() after the geometry and
     the particle source have been initialized.

     In our case we simple construct a scoring array with nreg+2 regions to
     collect the deposited energy in the nreg geometry regions and the reflected
     and transmitted energies, and if the user has requested it, initialize
     scoring array objects for pulse height distributions in
     the requested regions.
    */
    int initScoring();

    /*! Scale elastic scattering.
     The following method is used to modify the elastic scattering power in one
     or more given media. To do so, the user includes one or more lines in the
     scoring section of the input file:

        :start scoring options:
            scale xcc = MEDIUM_INDEX SCALING_FACTOR
        :stop scoring options:

        or

        :start scoring options:
            scale bc = MEDIUM_INDEX SCALING_FACTOR
        :stop scoring options:

     The effect of this will be that both, xcc and blcc will get multiplied with
     the provided factor, leaving the screening angle unchanged but increasing
     the number of collisions per unit length.
    */
    void setElasticScatteringScaling(
            EGS_Input *input_options,
            const char scaling);

    /*! Accumulate quantities of interest at run time
     This function is called from within the electron and photon transport
     routines at 28 different occasions specified by iarg (see PIRS-701
     for details). Here we are only interested in energy deposition =>
     only in iarg<=4 ausgab calls and simply use the score method of
     the scoring array object to accumulate the deposited energy.
    */
    int ausgab(int iarg);

    /*! Output intermediate results to the .egsdat file.
     This function is called at the end of each batch. We must store
     the results in the file so that simulations can be restarted and results
     of parallel runs recombined.
     */
    int outputData();

    /*! Read results from a .egsdat file.
     This function is used to read simulation results in restarted
     calculations.
     */
    int readData();

    /*! Reset the variables used for accumulating results
     This function is called at the beginning of the combineResults()
     function, which is used to combine the results of parallel runs.
    */
    void resetCounter();

    /*! Add simulation results
     This function is called from within combineResults() in the loop
     over parallel jobs. data is a reference to the currently opened
     data stream (basically the j'th .egsdat file).
     */
    int addState(istream &data);

    /*! Output the results of a simulation. */
    void outputResults();

    /*! Get the current simulation result.
     This function is called from the run control object in parallel runs
     in order to obtain a combined result for all parallel jobs.
     A single result is requested (and so, in simulations that calculate
     many quantites such as a 3D dose distribution, it is up to the user
     code to decide which single result to return). If this function is
     not reimplemented in a derived class, the run control object will simply
     not store information about the combined result in the JCF and not print
     this info in the log file. In our case we arbitrarily decide to return the
     reflected energy fraction as the single result of the simulation.
    */
    void getCurrentResult(
            double &sum,
            double &sum2,
            double &norm,
            double &count);

protected:

    /*! Start a new shower.
     This function is called from within the shower loop just before the
     actual simulation of the particle starts. The particle parameters are
     available via the protected base class variable p which is of type
     EGS_Particle (see egs_application.h).
     In our case we simply accumulate the total energy into Etot
     and, if the current history is different from the last, we call
     the startHistory() method of our scoring object to make known
     to the scoring object that a new history has started (needed for
     the history-by-history statistical analysis).
    */
    int startNewShower();

};

string AdvancedSimpleSlabCode::revision = "1.0";

#endif
