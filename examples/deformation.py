import faulthandler
faulthandler.enable()

import math

from bfm import Bfm, Condition, Force_linear, Instance, Mesh_lepl1110, Material, Obj, Rule_gauss_legendre, Sim
# create initial BFM context
# TODO should this be renamed something a little clearer, e.g. Scene?

bfm = Bfm()

# create a 2D simplex mesh
# this is simply a horizontal bar 1 unit wide and 0.2 units tall

# mesh = Mesh(2, Mesh.SIMPLEX)
#
# mesh.rect((0, 0), (1, 0.2))
# mesh.mesh()

mesh = Mesh_lepl1110("meshes/8.lepl1110")

sim = Sim.bfm_sim_read_lepl1110(mesh, "problem/problem.txt")
# create Dirichlet boundary conditions for mesh
# add all nodes close to the centre

boundary_condition = Condition(mesh, Condition.DIRICHLET)
# boundary_condition.populate(lambda mesh, coord: math.sqrt(sum(x ** 2 for x in coord)) < 0.1)
boundary_condition.populate(lambda mesh, coord: any(x < -0.499 for x in coord))

# create object out of 7075-series aluminium:
# density (rho): 2.81 g/cm^3
# Young's modulus (E): 71.7 GPa
# Poisson's ratio (nu): 0.33
# there are other properties such as tensile strength we could consider but this is enough for now
# for the material used in the LEPL1110 course: Material("Steel", rho=7.85e3, E=211.e9, nu=0.3)

material = Material.AA7075
rule = Rule_gauss_legendre(mesh.dim, mesh.kind)
obj = Obj(mesh, material, rule)

# create instance from object and boundary conditions
# add the instance to the state

instance = Instance(obj)
instance.add_condition(boundary_condition)

# create basic gravity force field

gravity = Force_linear.EARTH_GRAVITY_2D

# create simulation
# run the simulation

sim = Sim(Sim.DEFORMATION)

sim.add_instance(instance)
sim.add_force(gravity)

sim.run()

# display results
# resulting effects from the simulation will automatically be applied to the instance we added to our scene previously

bfm.show(sim)
