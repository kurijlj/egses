
###############################################################################
#
#  EGSnrc egs++ template cylindrical water phantom for PDD scoring.
#
#  Author: Ljubomir Kurij, 2019
#
#  Contributors:
#
###############################################################################
#
#  An template geometry input file for the egs++ geometry package.
#
###############################################################################


# Geometry setup
:start geometry definition:

    :start geometry:
        name        = pln_bnds
        library     = egs_planes
        type        = EGS_Zplanes
        positions   = 0.0 1.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 10.0 11.0 12.0 13.0 14.0 15.0 16.0 17.0 18.0 19.0 20.0 21.0 22.0 23.0 24.0 25.0 26.0 27.0 28.0 29.0 30.0 31.0 32.0 33.0 34.0 35.0 36.0 37.0 38.0 39.0 40.0
    :stop geometry:

    :start geometry:
        name        = cyl_bnds
        library     = egs_cylinders
        type        = EGS_ZCylinders
        radii       = 0.05 30.0
        midpoint    = 0
        :start media input:
            media = AIR521ICRU H2O521ICRU
            set medium = 0 1 1
            #set medium = 1 1
        :stop media input:
    :stop geometry:

    :start geometry:
        name            = the_phantom
        library         = egs_cdgeometry
        base geometry   = pln_bnds
        # set geometry = 1 geom means:
        # "in region 1 of the basegeometry, use geometry "geom"
        set geometry   = 0 40 cyl_bnds
        # The final region numbers are attributed by the cd geometry object;
        # Use the viewer to determine region numbers
    :stop geometry:

    simulation geometry = the_phantom

:stop geometry definition:


# Viewer setup
:start view control:

    # Here we are just assigning some colors for nice
    # display in the egs_view application
    set color = vacuum 0 0 0 0
    set color = AIR521ICRU        0 220 255  10
    ste color = H2O521ICRU        0 123 185 200
    set color = TA521ICRU       110 110 110 255
    set color = PB521ICRU       220 220 220 255
    set color = AL521ICRU       240 240 240 255
    set color = CU521ICRU       184 115  51 255
    set color = YELBRASS521ICRU 181 166  66 255
    set color = STEEL521ICRU     67  70  75 255
    set color = ZN521ICRU       186 196 200 255
    set color = TI521ICRU       130 135 136 255

:stop view control:
