
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
            media = air water
            set medium = 0 0
            set medium = 1 1
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
