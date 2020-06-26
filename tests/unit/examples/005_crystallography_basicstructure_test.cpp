#include "gtest/gtest.h"

#include "casm/crystallography/BasicStructure.hh"
#include "casm/crystallography/Lattice.hh"
#include "casm/crystallography/Molecule.hh"
#include "casm/crystallography/Site.hh"

TEST(ExampleCrystallographyBasicStructure, BasicStructureConstructor) {

  using Eigen::Vector3d;
  using CASM::xtal::BasicStructure;
  using CASM::xtal::Coordinate;
  using CASM::xtal::DoFSet;
  using CASM::xtal::Lattice;
  using CASM::xtal::Molecule;
  using CASM::xtal::Site;
  using CASM::xtal::SiteDoFSet;
  using CASM::AnisoValTraits;

  // A BasicStructure represents a crystal by specifying one unit cell.
  //
  //   BasicStructure has:
  //   - a Lattice
  //   - a basis (std::vector<Site>)
  //   - global DoF (std::map<DoFKey, xtal::DoFSet>)
  //
  //   The global DoF are represented by xtal::DoFSet, which is nearly equivalent to
  //     xtal::SiteDoFSet (xtal::DoFset does not have a list of excluded site occupants).

  // First, construct a Lattice
  Vector3d a {1.0, 0.0, 0.0};
  Vector3d b {0.0, 1.0, 0.0};
  Vector3d c {0.0, 0.0, 1.0};
  Lattice lattice {a, b, c};

  // Lambda function to make fractional Coordinate
  auto make_frac = [&](double x1, double x2, double x3) {
    return Coordinate { Vector3d {x1, x2, x3}, lattice, CASM::FRAC};
  };

  // Construct the BasicStructure, with an empty basis and no global DoF
  BasicStructure structure {lattice};

  // Construct atoms
  Molecule atom_A = Molecule::make_atom("A");
  Molecule atom_B = Molecule::make_atom("B");
  Molecule atom_C = Molecule::make_atom("C");
  Molecule atom_D = Molecule::make_atom("D");

  // Construct SiteDoFSet
  SiteDoFSet disp = AnisoValTraits::disp();

  // Construct the basis
  structure.set_basis({
    Site { make_frac(0.0, 0.0, 0.0), {atom_A, atom_B}, {disp} },
    Site { make_frac(0.5, 0.5, 0.0), {atom_A, atom_B}, {disp} },
    Site { make_frac(0.0, 0.5, 0.5), {atom_A, atom_B}, {disp} },
    Site { make_frac(0.5, 0.0, 0.5), {atom_C, atom_D}, {disp} } });

  // Add global DoF
  structure.set_global_dofs({ AnisoValTraits::strain("GL") }); // GLstrain: Green-Lagrange strain

  EXPECT_EQ(structure.basis().size(), 4);
  EXPECT_EQ(structure.global_dofs().size(), 1);

}
