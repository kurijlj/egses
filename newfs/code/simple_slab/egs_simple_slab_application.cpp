/*
###############################################################################
#
#  EGSnrc egs++ simple slab application
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
#  Contributors:    Reid Townson, Ljubomir Kurij
#
###############################################################################
*/


/*! \file egs_simple_slab_application.cpp
 *  \brief EGS_SimpleSlabApplication implementation
 *  \IK
 *
 * Also provides C-style functions needed to link with the mortran
 * back-end.
 */

#include "egs_simple_slab_application.h"
#include "egs_input.h"
#include "egs_base_source.h"
#include "egs_timer.h"
#include "egs_rndm.h"

#include "egs_interface2.h"

#include <cstring>
#include <cstdlib>

static EGS_SimpleSlabApplication  *egsApp = 0;

void _null_terminate(char *s, int len) {
    int j;
    for (j=len-1; j>=0; j--) {
        if (!isspace(s[j])) {
            if (j < len-1) {
                s[j+1] = 0;
            }
            break;
        }
    }
    if (j < 0) {
        s[0] = 0;
    }
}

const char *EGS_SimpleSlabApplication::egsHome() const {
    return the_egsio->egs_home;
}

const char *EGS_SimpleSlabApplication::henHouse() const {
    return the_egsio->hen_house;
}

const char *EGS_SimpleSlabApplication::pegsFile() const {
    return the_egsio->pegs_file;
}

const char *EGS_SimpleSlabApplication::inputFile() const {
    return the_egsio->input_file;
}

const char *EGS_SimpleSlabApplication::outputFile() const {
    return the_egsio->output_file;
}

const char *EGS_SimpleSlabApplication::userCode() const {
    return the_egsio->user_code;
}

const char *EGS_SimpleSlabApplication::workDir() const {
    return the_egsio->work_dir;
}

int EGS_SimpleSlabApplication::iParallel() const {
    return the_egsio->i_parallel;
}

int EGS_SimpleSlabApplication::nParallel() const {
    return the_egsio->n_parallel;
}

EGS_SimpleSlabApplication::EGS_SimpleSlabApplication(int argc, char **argv) {

    g = 0;
    source = 0;
    rndm = 0;
    input = 0;

    //
    // *** make sure that there is only a single application.
    //
    if (egsApp) egsFatal(
        "There can only be a single EGS_SimpleSlabApplication in a program\n"
    );

    egsApp = this;
    ncase = 0;

    //
    // *** set the number of histories to run
    //
    for (int i=0; i<argc-1; i++) {
        if (!strcmp("-n",argv[i]) || !strcmp("--ncase",argv[i])) {
            ncase = atoi(argv[i+1]);
            break;
        }
    }

    if (ncase < 1) {
        ncase = 1000;
    }

    //
    // *** init the EGS system
    //
    egsInit(argc, argv);

    //
    // *** null terminate directory and file names just in case
    //
    _null_terminate(the_egsio->user_code,64);
    _null_terminate(the_egsio->input_file,256);
    _null_terminate(the_egsio->output_file,256);
    _null_terminate(the_egsio->egs_home,128);
    _null_terminate(the_egsio->hen_house,128);
    _null_terminate(the_egsio->pegs_file,256);
    _null_terminate(the_egsio->work_dir,128);

    //
    // ********** Get the content of the input file.
    //
    // NOTE: The file in c++ code does not have to be referenced by absolute
    // NOTE: path. MOrtran and fortran code require files to be referenced with
    // NOTE: an absolute paths. So removing absolute path reference here won't
    // NOTE: enable using relative pathnames in the executable code.
    //
    string ifile(the_egsio->egs_home);
    ifile += the_egsio->user_code;
    ifile += path_separator;
    ifile += the_egsio->input_file;
    ifile += ".egsinp";
    input = new EGS_Input;
    input->setContentFromFile(ifile.c_str());

    //
    // ********** We disregard the simulation geometry from the input and set
    // ********** simulation geometry from a string.
    //
    string geom_spec = string (
        ":start geometry definition:\n"
        "   :start geometry:\n"
        "       library = egs_planes\n"
        "       type = EGS_Zplanes\n"
        "       name = planes\n"
        "       positions = 0.0 0.1 40.0\n"
        "   :stop geometry:\n"
        "   :start geometry:\n"
        "       library = egs_cylinders\n"
        "       type = EGS_ZCylinders\n"
        "       name = cylinder\n"
        "       radii = 20.0\n"
        "   :stop geometry:\n"
        "   :start geometry:\n"
        "       name = lab\n"
        "       library = egs_ndgeometry\n"
        "       dimensions = planes cylinder\n"
        "       :start media input:\n"
        "           media = TA521ICRU AIR521ICRU\n"
        "           set medium = 0 0\n"
        "           set medium = 1 1\n"
        "       :stop media input:\n"
        "   :stop geometry:\n"
        "   simulation geometry = lab\n"
        ":stop geometry definition:\n");

    EGS_Input geom_input;
    geom_input.setContentFromString (geom_spec);
    g = EGS_BaseGeometry::createGeometry (&geom_input);
    if (!g) {
        egsFatal ("Failed to construct the simulation geometry\n");
    }
    EGS_BaseGeometry::describeGeometries ();
    egsInformation ("\nThe simulation geometry is of type %s and has the "
                   "name '%s'\n\n",g->getType().c_str(),g->getName().c_str());

    //
    // *********** Add the media names present in the geometry
    //
    for (int j=0; j<g->nMedia(); j++) {
        const char *medname = g->getMediumName(j);
        int len = strlen(medname);
        int ind = egsAddMedium(medname, len);
        if (ind != j+1) {
            egsFatal("Medium index mismatch: %d %d\n",ind,j+1);
        }
    }

    //
    // ********** We disregard the particle source from the input and set
    // ********** particle source from a string.
    //
    string source_spec = string(
        ":start source definition:\n"
        "   :start source:\n"
        "       name = the_source\n"
        "       library = egs_parallel_beam\n"
        "       charge = 0\n"
        "       direction = 0 0 1\n"
        "       :start spectrum:\n"
        "           type = monoenergetic\n"
        "           energy = 6.0\n"
        "       :stop spectrum:\n"
        "       :start shape:\n"
        "           type = point\n"
        "           position = 0.0 0.0 0.0\n"
        "       :stop shape:\n"
        "   :stop source:\n"
        "   simulation source = the_source\n"
        ":stop source definition:\n");

    EGS_Input source_input;
    source_input.setContentFromString (source_spec);
    source = EGS_BaseSource::createSource (&source_input);
    if (!source) {
        egsFatal ("Failed to construct the particle source\n");
    }

    //
    // ********* Construct a random number generator from the input
    //
    rndm = EGS_RandomGenerator::createRNG(input,the_egsio->i_parallel);
    if (!rndm) {
        rndm = EGS_RandomGenerator::defaultRNG(the_egsio->i_parallel);
    }

    //
    // ********** Get transport parameter settings from the input file
    //
    int ounit = 6;
    egsGetTransportParameter(&ounit);

    //
    // *********** Get the cross section data
    //
    egsHatch();

    //
    // *********** Set ecut_new and pcut_new
    //
    the_bounds->ecut_new = the_bounds->ecut;
    the_bounds->pcut_new = the_bounds->pcut;

    //
    // ********** Now check if the cross section data covers the energy
    //            range needed by the source
    //
    EGS_Float Emax = source->getEmax();
    bool is_ok = true;
    for (int imed=0; imed<g->nMedia(); imed++) {
        if (Emax > the_thresh->up[imed] ||
                Emax > the_thresh->ue[imed] - the_useful->rm) {
            egsWarning("The maximum energy of the source (%g) is higher than\n"
                       "  the cross section data energy range for medium %d\n",
                       Emax,imed+1);
            is_ok = false;
        }
    }
    if (!is_ok) {
        egsFatal("Create new PEGS data sets and retry\n");
    }

    nreport = 10;

}

EGS_SimpleSlabApplication::~EGS_SimpleSlabApplication() {
    if (g) {
        if (g->deref()) {
            delete g;
        }
    }
    if (source) {
        delete source;
    }
    if (rndm) {
        delete rndm;
    }
    if (input) {
        delete input;
    }
}

void EGS_SimpleSlabApplication::finish() {
    egsFinish();
}

int EGS_SimpleSlabApplication::run() {

    egsInformation("\n\nSimulating %d particles...\n\n",ncase);
    EGS_Timer timer;
    timer.start();
    EGS_I64 nperb = ncase/nreport;
    if (nperb < 1) {
        nperb = 1;
    }
    EGS_Float aperb = ((EGS_Float)nperb)/((EGS_Float)ncase);
    int q, latch;
    EGS_Float E, wt;
    EGS_Vector x,u;
    sum_E = 0;
    sum_E2 = 0;
    Etot = 0;
    sum_w = 0;
    sum_w2 = 0;
    for (EGS_I64 icase=1; icase<=ncase; icase++) {
        if (icase%nperb == 0) {
            egsInformation(
                "+Finished %7.2f percent of cases, cpu time = %9.3f\n",
                100 * aperb * (icase / nperb),
                timer.time());
        }
        EGS_I64 this_case = source->getNextParticle(rndm,q,latch,E,wt,x,u);
        //egsInformation("Got %d %g %g (%g,%g,%g) (%g,%g,%g) %lld\n",
        //      q,E,wt,x.x,x.y,x.z,u.x,u.y,u.z,this_case);
        sum_E += E*wt;
        sum_E2 += wt*E*E;
        sum_w += wt;
        sum_w2 += wt*wt;
        int ireg = g->isWhere(x);
        if (ireg < 0) {
            EGS_Float t = veryFar;
            ireg = g->howfar(ireg,x,u,t);
            if (ireg >= 0) {
                x += u*t;
            }
        }
        if (ireg >= 0) {
            //egsInformation(" entering in region %d\n",ireg);
            last_case = this_case;
            if (q == 1) {
                Etot += (E + 2*the_useful->rm)*wt;
            }
            else {
                Etot += E*wt;
            }
            the_stack->E[0] = (q) ? E + the_useful->rm : E;
            the_stack->x[0] = x.x;
            the_stack->y[0] = x.y;
            the_stack->z[0] = x.z;
            the_stack->u[0] = u.x;
            the_stack->v[0] = u.y;
            the_stack->w[0] = u.z;
            the_stack->dnear[0] = 0;
            the_stack->wt[0] = wt;
            the_stack->ir[0] = ireg+2;
            the_stack->iq[0] = q;
            the_stack->latch[0] = latch;
            the_stack->np = 1;
            startHistory(this_case);
            egsShower();
            endHistory();
        }
    }
    egsInformation(
        "\n\nFinished simulation, CPU time was %g\n\n",
        timer.time());
    sum_E = sum_E/sum_w;
    sum_E2 = sum_E2/sum_w;
    sum_E2 -= sum_E*sum_E;
    if (sum_E2 > 0) {
        sum_E2 = sqrt(sum_E2/(sum_w*sum_w/sum_w2-1));
    }
    egsInformation("Average particle energy: %g +/- %g\n\n",sum_E,sum_E2);

    return 0;

}

void EGS_SimpleSlabApplication::fillRandomArray(int n, EGS_Float *rarray) {
    rndm->fillArray(n,rarray);
}

extern __extc__ void egsHowfar() {
    int np = the_stack->np-1;
    int ireg = the_stack->ir[np]-2;

    // Particle is outside the geometry so discard it.
    if (ireg < 0) {
        the_epcont->idisc = 1;
        return;
    }

    // Let's see in which region particle is going to be after step.
    int newmed;
    int inew = egsApp->howfar(
        ireg,
        EGS_Vector(the_stack->x[np], the_stack->y[np], the_stack->z[np]),
        EGS_Vector(the_stack->u[np], the_stack->v[np], the_stack->w[np]),
        the_epcont->ustep,
        &newmed);

    // Particle is going to end up in a new region, so set new region index.
    if (inew != ireg) {
        the_epcont->irnew = inew+2;
        the_useful->medium_new = newmed+1;

        // Particle is going to end up outside the geometry so terminate
        // particle at the end of a step.
        if (inew < 0) {
            the_epcont->idisc = -1; // i.e. discard after the step.
            the_useful->medium_new = 0;
        }
    }
}

extern __extc__ void egsAusgab(EGS_I32 *iarg) {
    egsApp->ausgab(*iarg);
}

extern __extc__ void egsHownear(EGS_Float *tperp) {
    int np = the_stack->np-1;
    int ireg = the_stack->ir[np]-2;
    *tperp = egsApp->hownear(ireg,EGS_Vector(the_stack->x[np],the_stack->y[np],
                             the_stack->z[np]));
}

extern __extc__ void egsStartParticle() {
    int np = the_stack->np - 1;
    int ir = the_stack->ir[np]-2;
    the_useful->medium = egsApp->getMedium(ir)+1;
    the_epcont->idisc = 0;
}

extern __extc__ void egsFillRandomArray(const EGS_I32 *n, EGS_Float *rarray) {
    egsApp->fillRandomArray(*n,rarray);
}

