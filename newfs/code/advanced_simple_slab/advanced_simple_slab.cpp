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

    // Retrieve scoring options from an input file.
    EGS_Input *options = input->takeInputItem("scoring options");

    // Process user options supplied in a *.egsinput file.
    if(options) {

        // Scale elastic scattering.
        setElasticScatteringScaling(options, 'x');
        setElasticScatteringScaling(options, 'b');

        vector<string> choices;
        choices.push_back("no");
        choices.push_back("yes");

        deflect_brems = options->getInput(
            "deflect electron after brems",
            choices,
            0);
        if(deflect_brems) {
            egsInformation(
                "\n *** Using electron deflection in brems events\n\n");
            setAusgabCall(AfterBrems, true);
        }

        int n_rr;
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

    while((scale_input = input_options->takeInputItem(input_item))) {
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
        }
        delete scale_input;
    }
}

int AdvancedSimpleSlabCode::ausgab(int iarg) {
    if(iarg <= 4) {
        int np = the_stack->np - 1;

        // Note: ir is the region number+1
        int ir = the_stack->ir[np] - 1;

        int iq = the_stack->iq[np];

        // If the particle is outside the geometry and headed in the positive
        // z-direction, change the region to count it as "transmitted"
        // Note: This is only valid for certain source/geometry conditions!
        // If those conditions are not met, the reflected and transmitted
        // energy fractions will be wrong
        if(ir == 0 && the_stack->w[np] > 0) {
            ir = nreg + 1;
        }

        EGS_Float aux = the_epcont->edep * the_stack->wt[np];
        if(aux > 0) {
            score->score(ir, aux);
        }

        if(ir == nreg + 1) {
            EGS_ScoringArray *flu = the_stack->iq[np] ? eflu : gflu;
            EGS_Float r2 =
                (the_stack->x[np] * the_stack->x[np]) +
                (the_stack->y[np] * the_stack->y[np]);

            if(r2 < 400) {
                int bin = (int) (sqrt(r2) * 10.0);

                aux = the_stack->wt[np] / the_stack->w[np];
                if(aux > 0) {
                    flu->score(bin, aux);
                }
            }
        }

        if(prev_case != current_case) {
            prev_case = current_case;
            np_hist.push_back(prev_case + 100);
        }

        if(iq && np_map[np] == 1 && iarg) {
            np_map[np] = 0;
            np_hist.push_back(-1 * ( np + 1 ));
        }
        else if (iq && iarg == 0) {
            np_map[np] = 1;
            np_hist.push_back((np + 1));
        }

        return 0;
    }

    int np = the_stack->np - 1;

    if(iarg == BeforeBrems ||
            iarg == BeforeAnnihRest ||
            iarg == BeforeAnnihFlight &&
            the_stack->latch[np] > 0) {
        the_stack->latch[np] = 0;
        rr_flag = 1;
        the_egsvr->nbr_split = the_egsvr->i_do_rr;

        return 0;
    }

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
    egsInformation(
        "\n\n last case = %d Etot = %g\n",
        (int)current_case, Etot);
    double norm = ((double)current_case) / Etot;
    egsInformation("NP history: \n");
    for(int item : np_hist) {
        cout << item << " ";
    }
    egsInformation("\n\n");
    egsInformation("\n\n======================================================\n");
    egsInformation(" Energy fractions\n");
    egsInformation("======================================================\n");
    egsInformation("The first and last items in the following list of energy fractions are the reflected and transmitted energy, respectively. These two values are only meaningful if the source is directed in the positive z-direction. The remaining values are the deposited energy fractions in the regions of the geometry, but notice that the identifying index is the region number offset by 1 (ir+1).");
    score->reportResults(
        norm,
        "ir+1 | Reflected, deposited, or transmitted energy fraction",
        false,
        "  %d  %12.6e +/- %12.6e %c\n");
    EGS_Float Rmax = 20;
    EGS_Float dr = Rmax / 200;
    /*
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
#endif
