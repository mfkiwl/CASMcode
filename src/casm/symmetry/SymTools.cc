#include <iterator>
#include <memory>
#include "casm/crystallography/SymTools.hh"
#include "casm/crystallography/Structure.hh"
#include "casm/crystallography/UnitCellCoord.hh"
#include "casm/symmetry/SymOp.hh"
#include "casm/symmetry/SymTools.hh"
#include "casm/symmetry/SymTools_impl.hh"
#include "casm/symmetry/SymGroup.hh"
#include "casm/symmetry/SymBasisPermute.hh"
#include "casm/misc/CASM_Eigen_math.hh"

namespace {
  CASM::SymGroup subgroup_from_indices(const CASM::SymGroup &super_group, const std::vector<CASM::Index> &subgroup_indices) {
    std::vector<CASM::SymOp> subgroup_operations;
    for(auto ix : subgroup_indices) {
      subgroup_operations.push_back(super_group[ix]);
    }

    return CASM::SymGroup(subgroup_operations, &(super_group.lattice()), super_group.periodicity());
  }

  //*******************************************************************************************
}
namespace CASM {
  namespace sym {
    SymGroup invariant_subgroup(const SymGroup &super_group, const xtal::Lattice &lat) {
      auto subgroup_operation_indices = xtal::invariant_subgroup_indices(lat, super_group);
      return subgroup_from_indices(super_group, subgroup_operation_indices);
    }

    template<>
    xtal::UnitCellCoord copy_apply<CASM::SymOp, xtal::UnitCellCoord, xtal::Structure>(const CASM::SymOp &op, xtal::UnitCellCoord copied_ucc, const xtal::Structure &prim) {

      // transform using stored SymBasisPermute representation
      const SymBasisPermute &rep = *op.get_basis_permute_rep(prim.basis_permutation_symrep_ID());
      xtal::UnitCell new_unitcell = rep.matrix() * copied_ucc.unitcell() + rep[copied_ucc.sublattice()].unitcell();
      auto new_sublattice = rep[copied_ucc.sublattice()].sublattice();

      // additional translations (such as needed for supercell factor groups),
      // are stored in SymOp::integral_tau() (in cartesian coordinates)
      // this converts that to fractional coordinates and adds it to the unitcell()
      new_unitcell += lround(prim.lattice().inv_lat_column_mat() * op.integral_tau());

      return xtal::UnitCellCoord(new_sublattice, new_unitcell);
    }

    template<>
    xtal::UnitCellCoord &apply<CASM::SymOp, xtal::UnitCellCoord, xtal::Structure>(const CASM::SymOp &op, xtal::UnitCellCoord &mutating_ucc, const xtal::Structure &prim) {
      auto ucc_after_apply = copy_apply(op, mutating_ucc, prim);
      std::swap(ucc_after_apply, mutating_ucc);
      return mutating_ucc;
    }


  } // namespace sym
} // namespace CASM
