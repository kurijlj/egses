/*
###############################################################################
#
#  EGSnrc egs++ simple_slab application
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
#  Contributors:    Ernesto Mainegra-Hing, Ljubomir Kurij
#
###############################################################################
#
#  A simple EGSnrc application using the C++ interface. It implements the
#  functionality of the original tutor2 tutorial code written in mortran
#  except that now, due to the use of the general geometry and source packages
#  included in egs++, any geometry or any source can be used, not just a
#  monoenergetic 20 MeV source of electrons incident on a 1 mm plate of
#  tantalum as un tutor2.
#
###############################################################################
*/


//! We derive from EGS_SimpleSlabApplication => we must include its header file
#include "egs_simple_slab_application.h"
//! We use a scoring array object => need its header file.
#include "egs_scoring.h"
//! egs_interface2.h must be included by any C++ application.
#include "egs_interface2.h"
//! To get access to egsInformation(), etc.
#include "egs_functions.h"

#include <stdio.h>  // required by EOF
#include <getopt.h> // required by getopt
#include <stdlib.h> // required by EXIT_SUCCESS

struct app_info_struct {
    const char *appname;
    const char *execname;
    const char *description;
    const char *help;
    const char *copyright;
    const char *version;
    const char *year;
    const char *author;
    const char *mail;
} info;

class APP_EXPORT SimpleSlab_Application : public EGS_SimpleSlabApplication {

    EGS_ScoringArray *edep;   // our scoring object
    EGS_ScoringArray *ecount; // our scoring object
    int              nreg;    // number of regions in the geometry.

public:

    /*! \brief Constructor.

     Simply calls the EGS_SimpleApplication constructor passing the command
     line arguments to it and then constructs a scorring array object that will
     collect the energy deposited in the various regions during run time.
     In the EGS_SimpleApplication constructor the input file is read in,
     geometry, source and random number generators are initialized and the
     \c egs_init function is called. The \c egs_init function sets default
     values, creates a temporary working directory for the run and opens
     various Fortran I/O units needed by the mortran back-end. The media
     present in the geometry are then added to the mortran back-end and the
     HATCH subroutine is called to initialize the cross section data.
     Finally, a check is made that the cross section data files cover
     the energy range needed based on the maximum energy of the source.
    */
    SimpleSlab_Application(int argc, char **argv) :
        EGS_SimpleSlabApplication (argc, argv) {

        nreg = g->regions ();
        /*! We initialize the scorring array to have 1 more regions than the
            geometry so that we can collect the transmitted and reflected
            energy fractions in additions to the energy fractions deposited
            in the nreg regions of the geometry.
        */
        edep = new EGS_ScoringArray (nreg + 1);

        // We score only electrons that are entering region behind plate
        // (iarg = 2) from the plate (iarg = 1), so we only need one item in the
        // scoring array.
        // ecount = new EGS_ScoringArray (1);
        ecount = new EGS_ScoringArray (nreg + 1);
    };

    /*! \brief Destructor.

     It is a good coding practice to deallocate memory when objects
     go out of scope. That's what we do in the destructor of our application.
     */
    ~SimpleSlab_Application() {
        delete edep;
        delete ecount;
    };

    /*! \brief Scoring.

     Every application derived from EGS_SimpleApplication must provide an
     implementation of the ausgab function to score the quantities of
     interest. For the possible values of the iarg argument see the
     EGSnrc manual (PIRS-701). In tutor2pp we are only interested in energy
     deposition => only need \a iarg <= 4 calls.  The particle stack is available to us via the pointer \c the_stack,
     the \c EPCONT common block via the pointer \c the_epcont.
     We have to remember that the stack pointer (\c the_stack->np), which
     points to the last particle on the stack, uses Fortran style indexing
     (\em i.e., it goes from 1 to np)
    */
    int ausgab(int iarg) {
        if (iarg <= 4) {
            //! Get the stack pointer and currect particle region index.
            int np = the_stack->np - 1;
            
            // NOTE: ir is the region number+1
            int ir = the_stack->ir[np]-1;
            /*! Per definition region index=0 corresponds to the outside,
               regions 1...nreg to the nreg regions inside the geometry.
               If the particle is outside, we say that it is 'reflected'
               if its direction cosine with respect to the z-axis is negative
               and use region 0 of the scoring array to score its energy.
               If the particle is outside but moving forward, we say that
               the particle is 'transmitted' and use region nreg+1 to score
               its energy.
            */
            if( ir == 0 && the_stack->w[np] > 0 ) ir = nreg+1;
            /*! Now simply use the score method of the EGS_ScoringArray class
                to record the energy deposited. */
            edep->score(ir,the_epcont->edep*the_stack->wt[np]);

            if (iarg == 0) {
                int iq = the_stack->iq[np];
                int irnew = the_epcont->irnew-1;
                if (-1 == iq && 1 == ir && 2 == irnew) {
                    ecount->score (irnew, 1);
                }
            }
        }
    };

    /*! \brief Statistics

     This function gets called before each new history.
     The argument icase is the history number returned from the
     source when sampling the next particle. In many cases this
     will be the same as the history number counter in the shower loop,
     but there are situations where this is not the case (e.g.
     for a phase space file source icase will be the number of statistically
     independent particles read so far from the file, which will be
     different than the number of particles read).
     We simply call here the setHistory() method of the scoring object.
     This is sufficient to get a history-by-history statistical analysis
     for the deposited energy fractions.
    */
    void startHistory(EGS_I64 icase) {
        edep->setHistory(icase);
        ecount->setHistory(icase);
    };


    /*! Output of results.

     Here we simply use the reportResults() method of the scoring array
     object, which requires as argumenst a normalization constant, a
     title (reproduced in the output),
     a boolean flag and a format string. The format string will be used
     to output the result and its uncertainty in each region. The flag
     determines if absolute uncertainties should be printed (false) or
     relative uncertainties (true). The result in each region is multiplied
     by the normalization constant. Note that the reportResults()
     method of the scoring object automatically
     divides by the number of statistically independent events.
     We want our results to be normalized as fractions of the energy
     imparted into the geometry => we need last_case/Etot as a normalization
     constant. The quantities last_case and Etot are collected by the
     EGS_Application base class during the run.
    */
    void reportResults() {
        double norm = ((double)last_case)/Etot;
        egsInformation(" last case = %d Etot = %g\n",(int)last_case,Etot);
        edep->reportResults (
            norm,
            "Reflected/deposited/transmitted energy fraction",
            false,
            "  %d  %9.5f +/- %9.5f %c\n");
        ecount->reportResults (
            1.0,
            "Number of electrons",
            true,
            "  %d  %9.5f +/- %9.5f %c\n");
    };

};

static void print_usage(int status) {
    if (0 != status)
        fprintf(
            stderr,
            "Try `%s --help' for more information.\n",
            info.execname
        );

    else {
        printf(
            "Usage: %s [- inputfile] [-p pegsfile] [-o outputfile]\n"
            "       [-n ncase] [-V, --version] [-h, --help]\n\n",
            info.execname
        );
    }
};

static void show_description(void) {
    printf("%s\n\n", info.description);
};

static void print_help(void) {
    printf(
        "%s\n\nReport %s bugs to %s\n",
        info.help,
        info.appname,
        info.mail
    );
}

static void show_version(void) {
    printf(
        "%s %s  Copyright (C) %s  %s\n%s\n",
        info.appname,
        info.version,
        info.year,
        info.author,
        info.copyright
    );
}

#ifdef BUILD_APP_LIB
APP_LIB(SimpleSlab_Application);
#else
//APP_MAIN(SimpleSlab_Application);
int main(int argc, char **argv) {

    info.appname = "SimpleSlab";
    info.execname = argv[0];
    info.description =
        "Application description goes here.";
    info.help =
        "Mandatory arguments to long options are mandatory for short options too.\n\n"
        "  -i inputfile     *.egsinp file\n"
        "  -p pegsfile      *.pegs4dat file\n"
        "  -o outputfile    file to output data to\n"
        "  -n ncase         number of showers to run\n"
        "  -h, --help       show this help message and exit\n"
        "  -V, --version    print program version";
    info.copyright =
        "This program comes with ABSOLUTELY NO WARRANTY; for details see <http://gnu.org/licenses/>.\n"
        "This is free software, and you are welcome to redistribute it\n"
        "under certain conditions; see <http://gnu.org/licenses/> for details.";
    info.version = "0.1";
    info.year = "2019";
    info.author = "Ljubomir Kurij";
    info.mail = "<kurijlj@gmail.com>";

    //
    // *** Parse command line options.
    //
    int option = -1;
    static struct option const long_options[] =
    {
        {NULL, required_argument, NULL, 'i'},
        {NULL, required_argument, NULL, 'p'},
        {NULL, required_argument, NULL, 'o'},
        {NULL, required_argument, NULL, 'n'},
        {"version", no_argument, NULL, 'V'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    while (EOF != (option = getopt_long(argc, argv, "i:p:o:n:Vh", long_options, NULL))) {
        switch(option) {
            case 'i':
                break;
            case 'p':
                break;
            case 'o':
                break;
            case 'n':
                break;
            case 'V':
                show_version();
                exit(EXIT_SUCCESS);
                break;
            case 'h':
                print_usage(EXIT_SUCCESS);
                show_description();
                print_help();
                exit(EXIT_SUCCESS);
                break;
            case '?':
                print_usage(EXIT_FAILURE);
                exit(EXIT_FAILURE);
        }
    }

    if (2 > argc) {
        printf("%s: No input file specified\n", info.execname);
        print_usage(EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }

    SimpleSlab_Application code(argc, argv);
    code.run();
    code.reportResults();
    code.finish();

    return EXIT_SUCCESS;
}
#endif

