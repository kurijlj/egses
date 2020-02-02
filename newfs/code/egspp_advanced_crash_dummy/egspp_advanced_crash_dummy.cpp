/*
###############################################################################
#
#  EGSnrc egs++ egs_app template application
#  Copyright (C) 2018 National Research Council Canada
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
#  Authors:         Frederic Tessier, 2018
#
#  Contributors:
#
###############################################################################
#
#  The simplest possible EGSnrc application, which relies on the implementation
#  of EGS_AdvancedApplication, to run a simulation described in an egs++ input
#  file (the provided slab.egsinp, for example).
#
#  The code commented below APP_MAIN can serve as a template for deriving an
#  application class from EGS_AdvancedApplication, and re-implementing the
#  initScoring method to select ausgab call triggers, and the ausgab method
#  to handle those calls.
#
###############################################################################
*/

#include "egspp_advanced_crash_dummy.h"
#include <cstdlib>
#include <fstream> // required by ofstream
//#include <string>
//#include <vector>
#include <assert.h>
#include "egs_advanced_application.h"
#include "egs_interface2.h"
#include "egs_scoring.h"
#include "egs_input.h"
#include "egs_rndm.h"
#include "egs_run_control.h"
#include "egs_ausgab_object.h"


// describeUserCode()
//
// Used to provide a description of your user code with output going to the
// log file.

void EGS_AdvancedCrushDummyApp::describeUserCode() const {
    egsInformation(
        "\n             ***************************************************"
        "\n             *                                                 *"
        "\n             *            Advanced Crash Dummy App             *"
        "\n             *                                                 *"
        "\n             ***************************************************"
        "\n\n");

    egsInformation(
        "This is egs++ Advanced Crash Dummy App based on "
        "EGS_AdvancedApplication.\n\n"
        "This code is a framework for testing behavior and performance of "
        "various parts\n"
        "of the egs++ code system.\n\n");
}


// describeSimulation()
//
// Used to provide a description of the simulation being performed with output
// going to the log file.
//
void EGS_AdvancedCrushDummyApp::describeSimulation() {
    EGS_AdvancedApplication::describeSimulation();
}


// initScoring()
//
// It is used to initialize the scoring of quantities of interest. In many cases
// the initialization will simply involve the construction of one or more
// EGS_ScoringArray objects, which will be used to accumulate the quantites
// of interest at run time. It is of course possible to have more advanced
// initialization controlled via a section in the input file that can be
// obtained from the #input protected data member (see e.g. cavity.cpp for
// an example). For our example case of a sode scoring application the
// implementation could look like this:
//
//     int MyDoseApplication::initScoring() {
//         dose = new EGS_ScoringArray(geometry->regions());
//         return 0;
//     }

int EGS_AdvancedCrushDummyApp::initScoring() {

    score = new EGS_ScoringArray(geometry->regions()+1);

    // call ausgab for all energy deposition events
    for (int call = BeforeTransport; call <= ExtraEnergy; ++call) {
        setAusgabCall((AusgabCall)call, true);
    }

    // don't call ausgab for other events
    for (int call = AfterTransport; call < UnknownCall; ++call) {
        setAusgabCall((AusgabCall)call, false);
    }

    // activate individual ausgab triggers (full list in myapplication.h). e.g.,
    setAusgabCall((AusgabCall) AfterTransport, true);
    setAusgabCall((AusgabCall) BeforeBrems, true);
    setAusgabCall((AusgabCall) AfterBrems,  true);

    return 0;
}


// resetCounter()
//
// This function is called before combining the results of a parallel run and
// should set all scoring objects to a 'pristine' state. In our example the
// implementation looks like this:
//
// void MyDoseApplication::resetCounter() {
//     EGS_AdvancedApplication::resetCounter();
//     dose->reset();
// }

void EGS_AdvancedCrushDummyApp::resetCounter() {
    EGS_AdvancedApplication::resetCounter();
    Etot = 0;
    score->reset();
}


// addState()
//
// This function is called from within the loop over parallel job results and
// should add the data found in the data stream passed as argument to the
// current data. In our example the implementation is as follows:
//
// int MyDoseApplication::addState(istream &data) {
//     // **** add base data
//     int err = EGS_AdvancedApplication::addState(data);
//     if(err) return err;
//     // *** temporary scoring array
//     EGS_ScoringArray tmp(geometry->regions());
//     if(!tmp.setState(data)) return 99; // error while reading data
//     (*dose) += tmp;                    // add data
//     return 0;
// }

int EGS_AdvancedCrushDummyApp::addState(istream &data) {
    int err = EGS_AdvancedApplication::addState(data);
    if(err) {
        return err;
    }

    double etot_tmp;
    data >> etot_tmp;
    Etot += etot_tmp;

    EGS_ScoringArray tmp(geometry->regions());
    if(!tmp.setState(data)) {
        return 101; // superclasses return values up to 13, so 101 should be
                    // resonably high error value for custom code to return
    }

    (*score) += tmp;

    return 0;
}


// startNewShower()
//
// In most cases the implementation will consist of calling
// the EGS_ScoringArray::setHistory() function of all scoring array objects
// used for accumulating results. This is necessary for the history-by-history
// statistical analysis. In our example the implementation should look
// like this:
//
// int MyDoseApplication::startNewShower() {
//     if(current_case != last_case) {
//         dose->setHistory(current_case);
//         last_case = current_case;
//     }
//     return 0;
// }

int EGS_AdvancedCrushDummyApp::startNewShower() {
    Etot += p.E*p.wt;
    if(current_case != last_case) {
        score->setHistory(current_case);
        last_case = current_case;
    }

    return 0;
}


// ausgab()
//
// Used to perform the actual scoring. Unfortunately, this is dependent upon the
// simulation back-end. For an application based on the mortran back-end,
// the relevant data is available from pointers to the Fortran common blocks
// with the general rule that the Fortran common block 'XXX' is accessed via
// the_xxx, e.g. the particle stack is pointed to by the_stack, the EPCONT
// common that contains information about energy deposition, step-lengths,
// region indeces, etc., via the_epcont, etc. Information about the various
// EGSnrc common blocks is found in PIRS-701. The common blocks exported as
// C-style structures are defined in \$HEN_HOUSE/interface/egs_interface2.h.
// Depending on the application, the ausgab() function may be quite complex.
// In our example application it is very simple:
//
// int MyDoseApplication::ausgab(int iarg) {
//     if( the_epcont->edep > 0 ) { // energy is being deposited
//         int np = the_stack->np - 1; // top particle on the particle stack.
//         int ir = the_stack->ir[np]-1; // region index
//         if( ir >= 0 ) dose->score(ir,the_epcont->edep*the_stack->wt[np]);
//     }
//     return 0;
// }
//
// In the above implementation no check is being made for the value of the
// iarg argument. This is because by default calls to ausgab() are made only for
// energy deposition events. However, in more advanced applciations, one can
// set up calls to ausgab for a number of other events (see PIRS-701,
// EGS_Application::AusgabCall, setAusgabCall()) and should check the value of
// iarg to determine the type of event initiating the call to ausgab(). Another
// thing to keep in mind is the fact that the Fortran-style indexing is used by
// the mortran back-end so that the top stack particle is the_stack->np - 1 and
// not the_stack->np. Finally, the convention used in the geometry package
// is that the outside region has index -1 while inside regions have indeces
// from 0 to the number of regions - 1. This convention is translated to the
// outside region being region 1 in the mortran back-end and inside regions
// having indeces 2 to number of regions + 1 and makes the subtraction of 2 from
// the particle region index necessary.

int EGS_AdvancedCrushDummyApp::ausgab(int iarg) {

    // stack index variables
    int np     = the_stack->np - 1;     // top particle on the particle stack
    int region = the_stack->ir[np] - 1; // region index

    // stack phase space variables
    int iq        = the_stack->iq[np];
    double E      = the_stack->E[np];
    double x      = the_stack->x[np];
    double y      = the_stack->y[np];
    double z      = the_stack->z[np];
    double u      = the_stack->u[np];
    double v      = the_stack->v[np];
    double w      = the_stack->w[np];
    double weight = the_stack->wt[np];

    // latch stack variable
    int latch = the_stack->latch[np];

    // handle the ausgab call
    if(4 >= iarg) {
        EGS_Float edep = the_epcont->edep * weight;
        if(0.0 < edep) {
            assert(0 <= region);
            score->score(region, edep);
        }
    }

    return 0;
}


// outputData()
//
// Used to output the current simulation results to a data file. This is
// necessary for the ability to restart a simulation or to combine the results
// of a parallel run. In most cases the implementation will involve calling the
// EGS_AdvancedApplication version first, so that data related to the state of
// the particle source, the random number generator, the EGSnrc back-end, etc.,
// is stored, and than calling the storeState() function of the scoring objects.
// In our example case the implementation should look like this:
//
// int MyDoseApplication::outputData() {
//     int err = EGS_AdvancedApplication::outputData();
//     if(err) return err;
//     if(!dose->storeState(*data_out)) return 99;
//     return 0;
// }
//
// i.e. if an error occurs while storing the base application data the function
// returns this error code. Otherwise it stores the dose data and returns
// a special error code if EGS_ScoringArray::storeState() fails.

int EGS_AdvancedCrushDummyApp::outputData() {
    int err = EGS_AdvancedApplication::outputData();
    if(err) {
        return err;
    }

    (*data_out) << "  " << Etot << endl;
    if(!score->storeState(*data_out)) {
        return 101; // superclasses return values up to 13, so 101 should be
                    // resonably high error value for custom code to return
    }

    return 0;
}


// readData()
//
// Used to read results from a data file. This is necessary for restarted
// calculations and is basically the same as outputData() but now readData() and
// setState() are used instead of outputData() and storeState():
//
// int MyDoseApplication::readData() {
//     int err = EGS_AdvancedApplication::readData();
//     if(err) return err;
//     if(!dose->setState(*data_in)) return 99;
//     return 0;
// }

int EGS_AdvancedCrushDummyApp::readData() {
    int err = EGS_AdvancedApplication::readData();
    if(err) {
        return err;
    }

    (*data_in) >> Etot;
    if(!score->setState(*data_in)) {
        return 101; // superclasses return values up to 13, so 101 should be
                    // resonably high error value for custom code to return
    }

    return 0;
}


// outputResults()
//
// The implementation should output the results of the simulation in a
// convenient form. Relatively short output could be put into the .egslog file
// by using the egsInformation() function for writing the data. Large amounts
// of data are better output into separate output files. To construct an output
// file name the application has access to the base output file name (without
// extension) via getOutputFile(), to the application directory via getAppDir(),
// to the working directory via getWorkDir(), etc.

void EGS_AdvancedCrushDummyApp::outputResults() {
    egsInformation(
        "\n\n" 
        "\n////////////////////////////////////////////////////////////////////"
        "////////////"
        "\n"
        "\n             ***************************************************"
        "\n             *                                                 *"
        "\n             *                Simulation Report                *"
        "\n             *                                                 *"
        "\n             ***************************************************"
        "\n"
        "\n////////////////////////////////////////////////////////////////////"
        "////////////"
        "\n");

    double norm = ((double)current_case) / Etot;
    egsInformation("\n\tNumber of regions: %d", geometry->regions());
    egsInformation("\n\t        Last case: %d", (int)current_case);
    egsInformation("\n\t             Etot: %g", Etot);
    egsInformation("\n\t             Norm: %g", norm);
    egsInformation("\n\n\tMedia by region:");
    egsInformation(
        "\n\t-------------------------------------------------------"
        "---------");
    egsInformation("\n\tir+1\tim\tMedia name");
    for (int i = 0; i < geometry->regions(); i++) {
        int im = geometry->medium(i); // Get media index.
        egsInformation("\n\t%d\t%d\t%s", i, im, geometry->getMediumName(i));
    }
    egsInformation(
        "\n\t-------------------------------------------------------"
        "---------");
    egsInformation("\n\n\tEnergy fractions:");
    egsInformation(
        "\n\t-------------------------------------------------------"
        "---------");
    egsInformation(
        "\n\tNotice that the identifying index of a region is the"
        "\n\tregion number offset by 1 (ir+1).");
    score->reportResults(
        norm,
        "\tir+1\tDeposited energy fraction",
        false,
        "\t%d\t%12.6e +/- %12.6e %c\n");
    egsInformation(
        "\n\t-------------------------------------------------------"
        "---------");
    egsInformation(
        "\n"
        "\n////////////////////////////////////////////////////////////////////"
        "////////////"
        "\n/ End of Report"
        "\n////////////////////////////////////////////////////////////////////"
        "////////////"
        "\n\n");
}


// getCurrentResult()
//
// Used to provide a single result and its uncertainty. This is not necessary
// for the proper operation of the application but is a nice feature. It is
// used to obtain a combined result for all parallel jobs during parallel
// processing execution, which is written to the job control file. In our
// example dose application one could for instance provide as the single result
// the dose in a single 'watch region':
//
// void MyDoseApplication::getCurrentResult(double &sum, double &sum2,
//                             double &norm, double &count) {
//     // set number of statistically independent events
//     count = current_case;
//     // obtain the score in the watch region
//     dose->currentScore(watch_region,sum,sum2);
//     // set the normalization
//     norm = 1.602e-10*count/source->getFluence()/watch_region_mass;
// }
//
// It is worth noting that in the process of combining the results of all
// parallel runs the run control object divides the result by the total number
// of statistically independent events. If we want the normalization to be in
// Gy/incident fluence we have to multiply back by count and then divide by the
// watch region mass, the particle fluence and multiply with 1.602e-10
// to convert the energy deposited (counted in MeV) to J.

void EGS_AdvancedCrushDummyApp::getCurrentResult(double &sum, double &sum2,
        double &norm, double &count) {
    count = current_case;             // Set number of statistically
                                      // independent events.
    norm = Etot > 0 ? count/Etot : 0; // Set the normalization.
    score->currentScore(0, sum, sum2);
}


int main(int argc, char **argv) {

    int r = EXIT_SUCCESS;
    EGS_AdvancedCrushDummyApp test_code(argc, argv);

    //r = test_code.initScoring();
    r = test_code.initSimulation();
    if(r) {
        return r;
    }
    r = test_code.runSimulation();
    if(r) {
        return r;
    }
    r = test_code.finishSimulation();
    if(r) {
        return r;
    }

    return r;
}
