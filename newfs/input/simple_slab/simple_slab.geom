#:start geometry:
#    library = egs_planes
#    type = EGS_Zplanes
#    name = planes
#    positions = 0.0 0.1 40.0
#:stop geometry:
#:start geometry:
#    library = egs_cylinders
#    type = EGS_ZCylinders
#    name = cylinder
#    radii = 20.0
#:stop geometry:
#:start geometry:
#    name = lab
#    library = egs_ndgeometry
#    dimensions = planes cylinder
#    :start media input:
#        media = TA521ICRU AIR521ICRU
#        set medium = 0 0
#        set medium = 1 1
#    :stop media input:
#:stop geometry:
#simulation geometry = lab
#
#
#:start source:
#    name = the_source
#    library = egs_parallel_beam
#    charge = 0
#    direction = 0 0 1
#    :start spectrum:
#        type = monoenergetic
#        energy = 6.0
#    :stop spectrum:
#    :start shape:
#        type = point
#        position = 0.0 0.0 0.0
#    :stop shape:
#:stop source:
#simulation source = the_source


:start geometry definition:
    :start geometry:
        library = egs_planes
        type = EGS_Zplanes
        name = planes
        positions = 0.0 0.1 40.0
    :stop geometry:
    :start geometry:
        library = egs_cylinders
        type = EGS_ZCylinders
        name = cylinder
        radii = 20.0
    :stop geometry:
    :start geometry:
        name = lab
        library = egs_ndgeometry
        dimensions = planes cylinder
        :start media input:
            media = TA521ICRU AIR521ICRU
            set medium = 0 0
            set medium = 1 1
        :stop media input:
    :stop geometry:
    simulation geometry = lab
:stop geometry definition:
