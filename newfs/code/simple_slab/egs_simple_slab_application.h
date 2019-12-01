/*
###############################################################################
#
#  EGSnrc egs++ simple slab application headers
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
#  Contributors:    Ljubomir Kurij
#
###############################################################################
*/


/*! \file egs_simple_application.h
 *  \brief EGS_SimpleApplication class header file
 *  \IK
 */

#ifndef EGS_SIMPLE_SLAB_APPLICATION_
#define EGS_SIMPLE_SLAB_APPLICATION_

#include "egs_libconfig.h"
#include "egs_base_geometry.h"

#ifdef WIN32
    const char path_separator = 92;
#else
    const char path_separator = '/';
#endif

class EGS_BaseSource;
class EGS_RandomGenerator;
class EGS_Input;

/*! \brief A base class for developing simple EGSnrc applications

  The easiest way to develop a C++ user code for EGSnrc is to derive
  from the EGS_SimpleApplication class and implement the ausgab() function
  to perform the scoring of the quantites of interest, possibly
  reimplement the startHistory() and endHistory() functions (called
  before/after the simulation of a single shower within the shower loop
  performed in run()), and implement initializitions and output of the
  results. An example of a simple C++ application for EGSnrc that uses
  a class derived from EGS_SimpleApplication is tutor2pp.

  A better way of developing C++ user codes for EGSnrc is to derive
  from the EGS_AdvancedApplication class.
*/
class APP_EXPORT EGS_SimpleSlabApplication {

public:

    //
    // ******************* constructor and destructor *************************
    //
    /*! \brief Construct a simple application object.

    The command line arguments must be passed to the EGS_SimpleApplication
    constructor so that the name of the application, the input file, the output
    file and the pegs file can be determined. The input file is then read in,
    geometry, source and random number generator are initialized and the
    egs_init function is called. The egs_init function, which is part of the
    mortran back-end, sets default values for all variables needed in the
    simulation, creates a temporary working directory for the run and
    opens various Fortran I/O units needed by the mortran back-end.
    The media present in the geometry are then added to the mortran back-end
    and the HATCH subroutine is called to initialize the cross section data.
    Finally, a check is made that the cross section data files cover
    the energy range needed based on the maximum energy of the source.
    */
    EGS_SimpleSlabApplication(int argc, char **argv);

    /*! Destructor.

    Deletes the geometry, the source, the random number generator and
    the input object.
    */
    virtual ~EGS_SimpleSlabApplication();

    //
    // ************************ geometry functions **********************
    //
    /*! \brief See the EGSnrc \link EGS_BaseGeometry::howfar() howfar
      geometry specification \endlink */
    inline int howfar(int ireg, const EGS_Vector &x, const EGS_Vector &u,
                      EGS_Float &t, int *newmed) {
        return g->howfar(ireg,x,u,t,newmed);
    };
    /*! \brief See the EGSnrc \link EGS_BaseGeometry::hownear() hownear
      geometry specification \endlink */
    inline EGS_Float hownear(int ireg,const EGS_Vector &x) {
        return g->hownear(ireg,x);
    };
    /*! \brief Get the medium index in region \a ireg */
    inline int getMedium(int ireg) {
        return g->medium(ireg);
    };

    //
    // ******************** ausgab ***************************************
    //
    /*! Scoring of results.

    This function must be reimplemented by the derived class to perform the
    scoring of the quantities of interest during the simulation.
    ausgab() is called with a corresponding integer argument on various
    events during the simulation (see PIRS-701).
    */
    virtual int ausgab(int) {
        return 0;
    };

    //
    // ********* finish the simulation
    //
    /*! \brief Finish the simulation.

    The default implemenmtation simply calls the egsFinish() function,
    which moves all output from the temporary working directory to the
    user code directory and deletes the temporary working directory.
    */
    virtual void finish();

    //
    // ********* run a simulation
    //
    /*! \brief Run the simulation.

    The default implementation of this function simulates #ncase showers,
    calling startHistory() and endHistory() before/after the shower is
    simulated. The progress of the simulation is reported #nreport
    times during the simulation. This number can bechanged using
    setNProgress().
    */
    virtual int run();

    //
    // ********* how often to report the progress of the simulation
    //
    /*! \brief Set the number of times the shower loop in run() reports
      the progress of the simulation to \a nprog.
     */
    void setNProgress(int nprog) {
        nreport = nprog;
    };

    //
    // ********* functions to be executed before and after a shower.
    //
    /*! \brief Start a new shower.

    This function is called from within the shower loop in run() before
    the simulation of the next particle starts. Could be reimplemented
    in derived classes to \em e.g. inform scoring objects that a new
    statistically independent event begins.
    */
    virtual void startHistory(EGS_I64) {};

    /*! Finish a shower.

    This function is called from within the shower loop in run() after
    the simulation of a particle has finished. Could be reimplemented in
    derived classes to \em e.g. score a pulse-height distribution.
    */
    virtual void endHistory() {};

    //
    // ******** fill an array with random numbers using the random
    //          number generator of this application.
    //
    /*! \brief Fill the array pointed to by \a r with \a n random numbers.
     */
    void fillRandomArray(int n, EGS_Float *r);

    //
    // ******** get various directories and file names
    //
    /*! Get the \c EGS_HOME directory */
    const char *egsHome() const;
    /*! Get the \c HEN_HOUSE directory */
    const char *henHouse() const;
    /*! Get the name of the PEGS file */
    const char *pegsFile() const;
    /*! Get the name of the input file */
    const char *inputFile() const;
    /*! Get the name of the output file */
    const char *outputFile() const;
    /*! Get the name of the user code */
    const char *userCode() const;
    /*! Get the working directory */
    const char *workDir() const;

    //
    // ******** the parallel run index and number of parallel jobs
    //
    /*! The index of this job in a parallel run */
    int iParallel() const;
    /*! Number of parallel jobs */
    int nParallel() const;

protected:

    EGS_I64             ncase;    //!< Number of showers to simulate
    int                 nreport;  //!< How often to report the progress
    EGS_BaseGeometry    *g;       //!< The simulation geometry
    EGS_BaseSource      *source;  //!< The particle source
    EGS_RandomGenerator *rndm;    //!< The random number generator.
    EGS_Input           *input;   //!< The input found in the input file.
    double              sum_E,  //!< sum of E*wt of particles from the source
                        sum_E2, //!< sum of E*E*wt of particles from the source
                        sum_w,  //!< sum of weights
                        sum_w2, //!< sum of weights squared
                        Etot;   /*!< same as sum_E but only for particles
                                     entering the geometry */
    EGS_I64             last_case; //!< last statistically independent event

};

#endif

#define APP_MAIN(app_name) \
    int main(int argc, char **argv) { \
        app_name app(argc,argv); \
        app.run(); \
        app.reportResults(); \
        app.finish(); \
        return 0; \
    }

#define APP_LIB(app_name) \
    extern "C" {\
        APP_EXPORT EGS_SimpleSlabApplication* createApplication(int argc, char **argv) {\
            return new app_name(argc,argv);\
        }\
    }

