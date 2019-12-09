/*
###############################################################################
#
#  EGSnrc egs++ egspp_crash_dummy application
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
#  Contributors:    Ljubomir Kurij, 2019
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
#  This code is used to test behaviour and performance of varoius parts of the
#  egs++ code framework.
#
###############################################################################
*/

#include "egs_advanced_application.h"
#include "egs_interface2.h"
#include "egs_input.h"

class APP_EXPORT EGS_CrashDummyApp : public EGS_AdvancedApplication {
    EGS_ScoringArray *score;

    public:
        EGS_CrashDummyApp(int argc, char **argv) :
            EGS_AdvancedApplication(argc,argv), score(0) {}
        ~EGS_CrashDummyApp() {
            if(score) delete score;
        };

        int initScoring();
        int ausgab(int iarg);

    private:
        void describeUserCode() const;
        void setElasticScatteringScaling(
                EGS_Input *input_options,
                const char scaling);

    protected:
        int initGeometry();
        int initSource();
};

