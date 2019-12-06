#include "advanced_simple_slab.h"     // We must include the app header file.
#include "egs_advanced_application.h" // We derive from EGS_AdvancedApplication.
#include "egs_scoring.h"              // Required by EGS_ScoringArray class.
#include "egs_interface2.h"           // Required by every egs++ application.
#include "egs_functions.h"            // Required by egsInformation function. It
                                      // requires const char * as input
                                      // parameter.
#include "egs_input.h"                // Required by EGS_Input class.
#include "egs_rndm.h"                 // Required by random number generator.

using namespace std;

extern "C" void F77_OBJ_(egs_scale_xcc, EGS_SCALE_XCC) (
        const int *,
        const EGS_Float *);
extern "C" void F77_OBJ_(egs_scale_bc, EGS_SCALE_BC) (
        const int *,
        const EGS_Float *);

void AdvancedSimpleSlabCode::describeUserCode() const {

    // If left empty make up an revision number for EGS_AdvancedApplication.
    string base_rev_info = " " == base_revision ? string("1.0") : base_revision;

    egsInformation(
        "\n               ***************************************************"
        "\n               *                                                 *"
        "\n               *           Advnaced Simple Slab Code             *"
        "\n               *                                                 *"
        "\n               ***************************************************"
        "\n\n");

    egsInformation(
    "This is Advanced Simple Slab Code v%s based on"
    " EGS_AdvancedApplication v%s\n\n",
    egsSimplifyCVSKey(revision).c_str(),
    egsSimplifyCVSKey(base_rev_info).c_str());
    egsInformation("\n\n");
}

int AdvancedSimpleSlabCode::initGeometry() {
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
    geometry = EGS_BaseGeometry::createGeometry (&geom_input);
    if (!geometry) {
        egsFatal ("Failed to construct the simulation geometry\n");
        return 1;
    }
    return 0;
}

int AdvancedSimpleSlabCode::initSource() {
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
        return 1;
    }
    return 0;
}

int AdvancedSimpleSlabCode::initScoring() {
    nreg = geometry->regions();
    score = new EGS_ScoringArray(nreg + 2);
    eflu = new EGS_ScoringArray(200); // Why are scorring arrays 200 items big?
    gflu = new EGS_ScoringArray(200); //

    // Fill map with default values (0 = we are not tracking stack index,
    // 1 = we are tracking stack index).
    np_map.fill(0);

    // Initialize with no russian roulette.
    the_egsvr->i_do_rr = 1;

    // We need to track particles after the step.
    setAusgabCall(AfterTransport, true);

    // Retrieve scoring options from an input file.
    EGS_Input *options = input->takeInputItem("scoring options");

    // Process user options supplied in a *.egsinput file.
    if(options) {

        // If set by user scale elastic scattering.
        setElasticScatteringScaling(options, 'x');
        setElasticScatteringScaling(options, 'b');

        vector<string> choices;
        choices.push_back("no");
        choices.push_back("yes");

        // Use electron deflection in brems events if set by user.
        deflect_brems = options->getInput(
            "deflect electron after brems",
            choices,
            0);
        if(deflect_brems) {
            egsInformation(
                "\n *** Using electron deflection in brems events\n\n");
            setAusgabCall(AfterBrems, true);
        }

        // If set use russian roulette.
        int n_rr = 1; // Initialize to no russian roulette.
        if(!options->getInput("Russian Roulette", n_rr) && n_rr > 1) {
            the_egsvr->i_do_rr = n_rr;
            setAusgabCall(BeforeBrems, true);
            setAusgabCall(AfterBrems, true);
            setAusgabCall(BeforeAnnihFlight, true);
            setAusgabCall(AfterAnnihFlight, true);
            setAusgabCall(BeforeAnnihRest, true);
            setAusgabCall(AfterAnnihRest, true);
            egsInformation(
                "\nUsing Russian Roulette with survival probability 1/%d\n",
                n_rr);
        }

        // We do not need options anymore so clean up.
        delete options;
    }

    return 0;
}

//
// Const char scaling can have one of the following values: 'x' for xcc scaling,
// and 'b' for bc (blcc) scaling.
//
void AdvancedSimpleSlabCode::setElasticScatteringScaling(
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
            if('b' == scaling) {
                F77_OBJ_(egs_scale_bc,EGS_SCALE_BC) (
                    &medium_index,
                    &parameters[1]);
            } else {
                F77_OBJ_(egs_scale_xcc,EGS_SCALE_XCC) (
                    &medium_index,
                    &parameters[1]);
            }
        } else {
            egsInformation("Error %d while reading items\n");
        }
        delete scale_input;
    }
}

int AdvancedSimpleSlabCode::ausgab(int iarg) {

    // Get stack pointer of the current particle.
    int np = the_stack->np - 1;

    if(iarg <= 4) {

        // Note: ir is the region number+1
        int ir = the_stack->ir[np] - 1;

        // We also need particle charge.
        int iq = the_stack->iq[np];

        // If the particle is outside the geometry and headed in the positive
        // z-direction, change the region to count it as "transmitted"
        // Note: This is only valid for certain source/geometry conditions!
        // If those conditions are not met, the reflected and transmitted
        // energy fractions will be wrong
        if(ir == 0 && the_stack->w[np] > 0) {
            ir = nreg + 1;
        }

        // We score only particles that deposit ebergy in the medium. If
        // deposited energy or statistical weight of particle is equal to zero
        // we don't score it.
        EGS_Float aux = the_epcont->edep * the_stack->wt[np];
        if(aux > 0) {
            score->score(ir, aux);
        }

        // Particle is at the back of a slab, so we need to score its fluence as
        // a function of the radial distance from z axis.
        if(ir == nreg + 1) {

            // Choose proper scoring array based on particle charge.
            EGS_ScoringArray *flu = the_stack->iq[np] ? eflu : gflu;

            // Calculate parrticle radial distance from z axis.
            EGS_Float r2 =
                (the_stack->x[np] * the_stack->x[np]) +
                (the_stack->y[np] * the_stack->y[np]);

            // Put particle in a corresponding energy bin according to particle
            // radial position.
            if(r2 < 400) {

                // Calculate bin.
                int bin = (int) (sqrt(r2) * 10.0);

                // Note exactly sure what are we scoring here?
                // It seems that we are dividing statistical weight of
                // a particle with its speed along z axis, and then we
                // score that!?
                aux = the_stack->wt[np] / the_stack->w[np];
                if(aux > 0) {
                    flu->score(bin, aux);
                }
            }
        }

        // If we have new history score history number incremented by 100.
        if(prev_case != current_case) {
            prev_case = current_case;
            np_hist.push_back(prev_case + 100);
        }

        // If we have a charged particle, it is not BeforeTransport and we are
        // already tracking it, now the particle history is terminated  and we
        // need to stop tracking it and report that particle is terminated.
        //
        // Else it is new particle on the stack or one we are already tracking
        // so score it.
        if(iq && iarg != 0 && np_map[np] == 1) {
            np_map[np] = 0;
            np_hist.push_back(-1 * ( np + 1 ));
        } else if (iq && iarg == 0) {
            if(!np_map[np]) np_map[np] = 1;
            np_hist.push_back((np + 1));
        }

        return 0;
    }

    // Not  sure what is going on here.
    if(iarg == BeforeBrems ||
            iarg == BeforeAnnihRest ||
            iarg == BeforeAnnihFlight &&
            the_stack->latch[np] > 0) {
        the_stack->latch[np] = 0;
        rr_flag = 1;
        the_egsvr->nbr_split = the_egsvr->i_do_rr;

        return 0;
    }

    // Handle electron deflection in brems events.
    if(iarg == AfterBrems && deflect_brems) {
        EGS_Vector u(
            the_stack->u[np-1],
            the_stack->v[np-1],
            the_stack->w[np-1]);
        EGS_Float tau = (the_stack->E[np-1] / the_useful->rm) - 1;
        EGS_Float beta = sqrt(tau * (tau + 2)) / (tau + 1);
        EGS_Float eta = (2 * rndm->getUniform()) - 1;
        EGS_Float cost = (beta + eta) / (1 + (beta * eta));
        EGS_Float sint = 1 - (cost * cost);
        if(sint > 0) {
            EGS_Float cphi, sphi;
            sint = sqrt(sint);
            rndm->getAzimuth(cphi, sphi);
            u.rotate(cost, sint, cphi, sphi);
            the_stack->u[np-1] = u.x;
            the_stack->v[np-1] = u.y;
            the_stack->w[np-1] = u.z;
        }
    }

    // Not  sure what is going on here.
    if(iarg == AfterBrems ||
       iarg == AfterAnnihRest ||
       iarg == AfterAnnihFlight) {
        the_egsvr->nbr_split = 1;
        if((iarg == AfterBrems) && rr_flag) {
            the_stack->latch[the_stack->npold-1] = 1;
        }
        rr_flag = 0;

        return 0;
    }

    return 0;
}

int AdvancedSimpleSlabCode::outputData() {
    // We first call the outputData() function of our base class.
    // This takes care of saving data related to the source, the random
    // number generator, CPU time used, number of histories, etc.
    int err = EGS_AdvancedApplication::outputData();
    if(err) return err;
    // We then write our own data to the data stream. data_out is
    // a pointer to a data stream that has been opened for writing
    // in the base class.
    (*data_out) << "  " << Etot << endl;
    if(!score->storeState(*data_out)) return 101;
    if(!eflu->storeState(*data_out)) return 301;
    if(!gflu->storeState(*data_out)) return 302;

    return 0;
}

int AdvancedSimpleSlabCode::readData() {
    // We first call the readData() function of our base class.
    // This takes care of reading data related to the source, the random
    // number generator, CPU time used, number of histories, etc.
    // (everything that was stored by the base class outputData() method).
    int err = EGS_AdvancedApplication::readData();
    if(err) return err;
    // We then read our own data from the data stream.
    // data_in is a pointer to an input stream that has been opened
    // by the base class.
    (*data_in) >> Etot;
    if(!score->setState(*data_in)) return 101;
    if(!eflu->setState(*data_in)) return 301;
    if(!gflu->setState(*data_in)) return 302;

    return 0;
}

void AdvancedSimpleSlabCode::resetCounter() {
    // Reset everything in the base class
    EGS_AdvancedApplication::resetCounter();
    // Reset our own data to zero.
    Etot = 0;
    score->reset();
    eflu->reset();
    gflu->reset();
}

int AdvancedSimpleSlabCode::addState(istream &data) {
    // Call first the base class addState() function to read and add
    // all data related to source, RNG, CPU time, etc.
    int err = EGS_AdvancedApplication::addState(data);
    if(err) return err;
    // Then read our own data to temporary variables and add to
    // our results.
    double etot_tmp;
    EGS_ScoringArray tmp(nreg + 2);
    data >> etot_tmp;
    Etot += etot_tmp;
    if(!tmp.setState(data)) return 101;
    (*score) += tmp;
    EGS_ScoringArray tmp1(200);
    if(!tmp1.setState(data)) return 301;
    (*eflu) += tmp1;
    if(!tmp1.setState(data)) return 302;
    (*gflu) += tmp1;

    return 0;
}

void AdvancedSimpleSlabCode::outputResults() {

    // Calculate value to normalize to.
    double norm = ((double)current_case) / Etot;

    // Print report for total values.
    egsInformation(
        "\n\n last case = %d\tEtot = %g\tnorm = %g\n\n\n\n",
        (int)current_case,
        Etot,
        norm);

    egsInformation("==========================================================="
        "=====================\n");
    egsInformation(" Energy fractions\n");
    egsInformation("==========================================================="
        "=====================\n");
    egsInformation(
"The first and last items in the following list of energy fractions are the\n"
"reflected and transmitted energy, respectively. These two values are only\n"
"meaningful if the source is directed in the positive z-direction. The\n"
"remaining values are the deposited energy fractions in the regions of\n"
"the geometry, but notice that the identifying index is the region number\n"
"offset by 1 (ir+1).");
    score->reportResults(
        norm,
        "ir+1 | Reflected, deposited, or transmitted energy fraction",
        false,
        "  %d  %12.6e +/- %12.6e %c\n");

    egsInformation("==========================================================="
        "=====================\n");
    egsInformation(" NP history: \n");
    egsInformation("==========================================================="
        "=====================\n");
    for(int item : np_hist) {
        cout << item << " ";
    }

    /*
    EGS_Float Rmax = 20;
    EGS_Float dr = Rmax / 200;
    egsInformation("\n\n Electron/Photon fluence at back of geometry as a function of radial distance\n"
                        "============================================================================\n");
    for(int j=0; j<200; ++j) {
        double fe,dfe,fg,dfg;
        eflu->currentResult(j,fe,dfe); gflu->currentResult(j,fg,dfg);
        EGS_Float r1 = dr*j, r2 = r1 + dr;
        EGS_Float A = M_PI*(r2*r2 - r1*r1);
        EGS_Float r = j > 0 ? 0.5*(r1 + r2) : 0;
        egsInformation("%9.3f  %15.6e  %15.6e  %15.6e  %15.6e\n",r,fe/A,dfe/A,fg/A,dfg/A);
    }
    */
}

void AdvancedSimpleSlabCode::getCurrentResult(
        double &sum,
        double &sum2,
        double &norm,
        double &count) {
    count = current_case;
    norm = Etot > 0 ? count / Etot : 0;
    score->currentScore(0, sum, sum2);
}

int AdvancedSimpleSlabCode::startNewShower() {
    Etot += p.E * p.wt;
    int res = EGS_Application::startNewShower();
    if(res) return res;
    if(current_case != last_case) {
        score->setHistory(current_case);
        eflu->setHistory(current_case);
        gflu->setHistory(current_case);
        last_case = current_case;
    }
    current_weight = p.wt;

    return 0;
}

#ifdef BUILD_APP_LIB
APP_LIB(AdvancedSimpleSlabCode);
#else
APP_MAIN(AdvancedSimpleSlabCode);
// Where APP_MAIN is defined in the egs_application.h as:
//     #define APP_MAIN(app_name)
//         int main(int argc, char **argv) {
//             app_name app(argc,argv);
//             int err = app.initSimulation();
//             if( err ) return err;
//             err = app.runSimulation();
//             if( err < 0 ) return err;
//             return app.finishSimulation();
#endif
