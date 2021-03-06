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

#include "egspp_crash_dummy.h"
#include <cstdlib>
#include <vector>
#include "egs_advanced_application.h"
#include "egs_interface2.h"
#include "egs_input.h"

// describeUserCode
void EGS_CrashDummyApp::describeUserCode() const {
    egsInformation(
        "\n             ***************************************************"
        "\n             *                                                 *"
        "\n             *                Crash Dummy App                  *"
        "\n             *                                                 *"
        "\n             ***************************************************"
        "\n\n");

    egsInformation(
        "This is egs++ Crash Dummy App based on EGS_AdvancedApplication.\n\n"
        "This code is a framework for testing behavior and performance of "
        "various parts\n"
        "of the egs++ code system.\n\n");
}

// setElasticScatteringScaling
//
// Const char scaling can have one of the following values: 'x' for xcc scaling,
// and 'b' for bc (blcc) scaling.
//
void EGS_CrashDummyApp::setElasticScatteringScaling(
        EGS_Input *input_options,
        const char scaling) {

    EGS_Input     *scale_input;
    string         option;
    string         input_item;
    string         info_message;

    if('b' == scaling) {
        option = string("bc");
    } else {
        option = string("xcc");
    }

    input_item = string("scale ").append(option);
    info_message =
        string("Scaling ") +
        option +
        string(" of medium %d with %g\n");

    egsInformation("Reading scoring parameters ...\n\n");

    while((scale_input = input_options->takeInputItem(input_item))) {
        egsInformation("Cycling through items ...\n");
        vector<EGS_Float> parameters;
        int error = scale_input->getInput(input_item, parameters);
        if(!error) {
            int medium_index = (int) parameters[0]; ++medium_index;
            egsInformation(info_message.c_str(), medium_index, parameters[1]);
        } else {
            egsInformation("Error %d while reading items\n");
        }
        delete scale_input;
    }
}

// initScoring
int EGS_CrashDummyApp::initScoring() {

    describeUserCode();
    egsInformation("Initializing ...\n\n");

    // call ausgab for all energy deposition events
    for (int call=BeforeTransport; call<=ExtraEnergy; ++call) {
        setAusgabCall((AusgabCall)call, true);
    }

    // don't call ausgab for other events
    for (int call=AfterTransport; call<UnknownCall; ++call) {
        setAusgabCall((AusgabCall)call, false);
    }

    // activate individual ausgab triggers (full list in myapplication.h). e.g.,
    setAusgabCall((AusgabCall) BeforeBrems, true);
    setAusgabCall((AusgabCall) AfterBrems,  true);

    // Retrieve scoring options from an input file.
    EGS_Input *options = input->takeInputItem("scoring options");

    // Process user options supplied in a *.egsinput file.
    if(options) {

        // Set elastic scattering scaling.
        setElasticScatteringScaling(options, 'x');
        setElasticScatteringScaling(options, 'b');

        delete options;
    }

    return 0;
}


// ausgab
int EGS_CrashDummyApp::ausgab(int iarg) {

    // stack index variables
    int np     = the_stack->np - 1;
    int npold  = the_stack->npold - 1;
    int region = the_stack->ir[np] - 2;

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

    return 0;
}

int main(int argc, char **argv) {
    EGS_CrashDummyApp test_code(argc, argv);
    test_code.initScoring();

    return EXIT_SUCCESS;
}
