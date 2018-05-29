#ifndef CASM_DiffusionTransformation
#define CASM_DiffusionTransformation

#include "casm/misc/cloneable_ptr.hh"
#include "casm/misc/Comparisons.hh"
#include "casm/symmetry/SymCompare.hh"
#include "casm/crystallography/UnitCellCoord.hh"
#include "casm/clusterography/ClusterInvariants.hh"
#include "casm/clusterography/IntegralCluster.hh"
#include "casm/clex/HasCanonicalForm.hh"
#include "casm/kinetics/DiffusionTransformationTraits.hh"
#include "casm/kinetics/DoFTransformation.hh"
#include "casm/kinetics/OccupationTransformation.hh"

namespace CASM {

  class AtomSpecie;
  class Structure;
  class Configuration;
  class SymOp;
  class jsonParser;
  class PermuteIterator;
  template<typename T> struct jsonConstructor;

  namespace Kinetics {

    /// \brief Specifies a particular specie
    /// A SpecieLocation object describes a particular specie at a specific
    /// lattice + basis site within the infinite crystal
    /// contains the UnitCellCoord (b, i, j, k) and occupant index (integer describing
    /// configurational degrees of freedom)
    struct SpecieLocation : public Comparisons<CRTPBase<SpecieLocation>> {

      /// Constructor Create SpecieLocation using a bijk and
      /// an occ index into that basis site's occupant array
      SpecieLocation(const UnitCellCoord &_uccoord, Index _occ, Index _pos);

      //THESE FIELDS SHOULD REALLY BE PRIVATE and have setters and getters
      /// Unitcell coordinate of where this resides
      UnitCellCoord uccoord;

      /// Occupant index
      Index occ;

      /// Position of specie in Molecule
      Index pos;

      /// Lexicographical comparison of SpecieLocations for sorting purposes
      bool operator<(const SpecieLocation &B) const;

      const Molecule &mol() const;

      const AtomSpecie &specie() const;

    private:

      std::tuple<UnitCellCoord, Index, Index> _tuple() const;
    };

    /// \brief Prints the information contained within this SpecieLocation
    /// b, i j k : occ pos
    std::ostream &operator<<(std::ostream &sout, const SpecieLocation &obj);

  }

  jsonParser &to_json(const Kinetics::SpecieLocation &obj, jsonParser &json);

  template<>
  struct jsonConstructor<Kinetics::SpecieLocation> {

    static Kinetics::SpecieLocation from_json(const jsonParser &json, const Structure &prim);
  };

  void from_json(Kinetics::SpecieLocation &obj, const jsonParser &json);


  namespace Kinetics {

    /// \brief Describes how one specie moves
    /// A SpecieTrajectory object tracks a singular species on its path from
    /// one site to another within the infinite crystal
    /// it contains two SpecieLocations from and to representing the initial and
    /// final site the species is found on. Both SpecieLocation objects should
    /// have the same Specie Information (a single atom is moving)
    class SpecieTrajectory : public Comparisons<CRTPBase<SpecieTrajectory>> {

    public:
      /// Constructor Creates SpecieTrajectory from two SpecieLocation objects
      /// SpecieLocation objects should have the same Specie information
      /// transforming a Ni into an Al does not make sense
      SpecieTrajectory(const SpecieLocation &_from, const SpecieLocation &_to);

      /// Rigidly shifts all sites within a SpecieTrajectory
      /// by a lattice translation
      SpecieTrajectory &operator+=(UnitCell frac);

      /// Rigidly shifts (negatively) all sites within a
      /// SpecieTrajectory by a lattice translation
      SpecieTrajectory &operator-=(UnitCell frac);

      /// Tells whether or not the SpecieTrajectory is valid due to
      /// having the same Specie moving
      bool specie_types_map() const;

      /// Tells whether or not the SpecieTrajectory is moving a species or not
      /// true indicates the trajectory is useless
      bool is_no_change() const;

      /// \brief Gives the starting coordinate of the specie moving
      UnitCellCoord from_loc() const;
      /// \brief Gives the ending coordinate of the specie moving
      UnitCellCoord to_loc() const;
      /// \brief Gives the name of the specie moving
      AtomSpecie specie() const;


      SpecieLocation from;
      SpecieLocation to;

      /// Lexicographical comparison of SpecieTrajectories for sorting purposes
      bool operator<(const SpecieTrajectory &B) const;

      /// Apply symmetry to locations within the trajectory
      SpecieTrajectory &apply_sym(const SymOp &op);

      /// Swaps the direction of the trajectory
      void reverse();

    private:

      std::tuple<SpecieLocation, SpecieLocation> _tuple() const;

    };
  }

  jsonParser &to_json(const Kinetics::SpecieTrajectory &traj, jsonParser &json);

  template<>
  struct jsonConstructor<Kinetics::SpecieTrajectory> {

    static Kinetics::SpecieTrajectory from_json(const jsonParser &json, const Structure &prim);
  };

  void from_json(Kinetics::SpecieTrajectory &traj, const jsonParser &json);


  namespace Kinetics {

    class DiffusionTransformation;

    /// \brief Invariants of a DiffusionTransformation, used to sort orbits
    class DiffTransInvariants {

    public:

      DiffTransInvariants(const DiffusionTransformation &trans);
      /// Upon application of symmetry the cluster size&shape does not change
      /// as well as the species that are present
      ClusterInvariants<IntegralCluster> cluster_invariants;
      std::map<AtomSpecie, Index> specie_count;

    };
  }


  /// \brief Check if DiffTransInvariants are equal
  bool almost_equal(const Kinetics::DiffTransInvariants &A,
                    const Kinetics::DiffTransInvariants &B,
                    double tol);

  /// \brief Compare DiffTransInvariants
  bool compare(const Kinetics::DiffTransInvariants &A,
               const Kinetics::DiffTransInvariants &B,
               double tol);

  /// \brief Print DiffTransInvariants
  std::ostream &operator<<(std::ostream &sout,
                           const Kinetics::DiffTransInvariants &obj);


  namespace Kinetics {

    typedef DoFTransformation <
    CanonicalForm <
    Comparisons <
    Translatable <
    SymComparable <
    CRTPBase<DiffusionTransformation >>> >>> DiffTransBase;

    /// \brief Describes how species move
    /// A DiffusionTransformation object is an object that represents
    /// a series of atoms(or vacancies) moving from sites on an infinite crystal
    /// to other sites on the crystal. There can be multiple ways of examining a
    /// DiffusionTransformation viewing a single site and watching the species that
    /// move through, or tracking a single species on the sites that it moves through.
    class DiffusionTransformation : public DiffTransBase {

    public:
      /// Constructor Create a null DiffusionTransformation on an infinite
      /// crystal represented by a structure
      DiffusionTransformation(const Structure &_prim);

      /// Return the tiling unit of the infinite crystal
      const Structure &prim() const;
      /// Rigid Shift all the coordinates in the DiffusionTransformation by
      /// a lattice translation
      DiffusionTransformation &operator+=(UnitCell frac);

      /// Checks to see if the species are compatible with a given site
      /// according to the prim degrees of freedom
      bool is_valid_occ_transform() const;

      /// \brief Check specie_types_map() && !breaks_indivisible_mol() && !is_subcluster_transformation()
      /// Ensures that the DiffusionTransformation is compatible with itself
      bool is_valid_specie_traj() const;

      /// Checks to see if the sub objects contain the species required
      bool specie_types_map() const;

      /// Checks to see if the transformation treats molecules illegally
      bool breaks_indivisible_mol() const;

      /// Checks to see if the transformation can be represented by
      /// one or more smaller transformations
      bool is_subcluster_transformation() const;

      /// \brief Check if specie_traj() and occ_transform() are consistent
      bool is_self_consistent() const;

      /// Performs all validity checks to see if DiffusionTransformation makes sense physically
      bool is_valid() const;

      /// Non-const access to OccupationTransformation vector
      /// (view point of sites and species moving through them)
      std::vector<OccupationTransformation> &occ_transform();
      /// Const access to OccupationTransformation vector
      /// (view point of sites and species moving through them)
      const std::vector<OccupationTransformation> &occ_transform() const;
      /// Non-const access to SpecieTrajectory vector
      /// (view point of tracking a single species and locations it moves through)
      std::vector<SpecieTrajectory> &specie_traj();
      /// Const access to SpecieTrajectory vector
      /// (view point of tracking a single species and locations it moves through)
      const std::vector<SpecieTrajectory> &specie_traj() const;
      /// Gives the cluster (sites only) that this Diffusion Transformation
      /// lives on
      const IntegralCluster &cluster() const;
      /// Gives a map from type of atom to amount in this DiffusionTransformation
      const std::map<AtomSpecie, Index> &specie_count() const;

      /// \brief Compare DiffusionTransformation
      /// Lexicographical Comparison for sorting purposes
      /// - Comparison is made using the sorted forms
      bool operator<(const DiffusionTransformation &B) const;

      Permutation sort_permutation() const;

      /// Puts Transformation in sorted form
      DiffusionTransformation &sort();

      /// Gives a sorted version of this
      DiffusionTransformation sorted() const;

      /// Tells whether this is sorted or not
      bool is_sorted() const;

      /// \brief Return the cluster size
      Index size() const;

      /// \brief Return the min pair distance, or 0.0 if size() <= 1
      double min_length() const;

      /// \brief Return the max pair distance, or 0.0 if size() <= 1
      double max_length() const;

      ///Applies symmetry to the coordinates of this Transformation
      /// updates occ_transform and specie_trajectory accordingly
      DiffusionTransformation &apply_sym(const SymOp &op);

      ///Applies symmetry using permute_iterator
      DiffusionTransformation &apply_sym(const PermuteIterator &it);

      /// Apply this transformation to a Configuration to
      /// Return a configuration with altered occupation
      Configuration &apply_to(Configuration &config) const;

      /// swaps the direction of the movement of species
      /// makes the final state the initial state
      void reverse();


    private:

      friend DiffTransBase;

      Configuration &apply_reverse_to_impl(Configuration &config) const;

      void _forward_sort();

      bool _lt(const DiffusionTransformation &B) const;

      /// \brief Reset mutable members, cluster and invariants, when necessary
      void _reset();

      std::map<AtomSpecie, Index> _from_specie_count() const;
      std::map<AtomSpecie, Index> _to_specie_count() const;

      const Structure *m_prim_ptr;

      std::vector<OccupationTransformation> m_occ_transform;
      std::vector<SpecieTrajectory> m_specie_traj;

      // stores IntegralCluster, based on occ_transform uccoord
      mutable notstd::cloneable_ptr<IntegralCluster> m_cluster;

      // stores Specie -> count, using 'from' specie
      // - is equal to 'to' specie count if is_valid_occ_transform() == true
      mutable notstd::cloneable_ptr<std::map<AtomSpecie, Index> > m_specie_count;

    };

    /// \brief Print DiffusionTransformation to stream, using default Printer<Kinetics::DiffusionTransformation>
    std::ostream &operator<<(std::ostream &sout, const DiffusionTransformation &trans);

    /// \brief Return a standardized name for this diffusion transformation orbit
    //std::string orbit_name(const PrimPeriodicDiffTransOrbit &orbit);

    // \brief Returns the distance from uccoord to the closest point on a linearly
    /// interpolated diffusion path considers the shortest path across PBC. (Could be an end point)
    double dist_to_path_pbc(const DiffusionTransformation &diff_trans, const UnitCellCoord &uccoord, const Supercell &scel);

    // \brief Returns the vector from uccoord to the closest point on a linearly
    /// interpolated diffusion path considers the shortest path across PBC. (Could be an end point)
    Eigen::Vector3d vector_to_path_pbc(const DiffusionTransformation &diff_trans, const UnitCellCoord &uccoord, const Supercell &scel);

    // \brief Returns the distance from uccoord to the closest point on a linearly
    /// interpolated diffusion path. (Could be an end point)
    double dist_to_path(const DiffusionTransformation &diff_trans, const UnitCellCoord &uccoord);

    // \brief Returns the vector from uccoord to the closest point on a linearly
    /// interpolated diffusion path. (Could be an end point)
    Eigen::Vector3d vector_to_path(const DiffusionTransformation &diff_trans, const UnitCellCoord &uccoord);

    /// \brief Determines which site is closest to the diffusion transformation and the vector to take it to the path
    std::pair<UnitCellCoord, Eigen::Vector3d> _path_nearest_neighbor(const DiffusionTransformation &diff_trans) ;

    /// \brief Determines which site is closest to the diffusion transformation
    UnitCellCoord path_nearest_neighbor(const DiffusionTransformation &diff_trans);

    /// \brief Determines the nearest site distance to the diffusion path
    double min_dist_to_path(const DiffusionTransformation &diff_trans);

    /// \brief Determines the vector from the nearest site to the diffusion path in cartesian coordinates
    Eigen::Vector3d min_vector_to_path(const DiffusionTransformation &diff_trans);

    /// \brief Determines whether the atoms moving in the diffusion transformation will collide on a linearly interpolated path
    bool path_collision(const DiffusionTransformation &diff_trans);

  }

  /// \brief Write DiffusionTransformation to JSON object
  jsonParser &to_json(const Kinetics::DiffusionTransformation &trans, jsonParser &json);

  template<>
  struct jsonConstructor<Kinetics::DiffusionTransformation> {

    static Kinetics::DiffusionTransformation from_json(const jsonParser &json, const Structure &prim);
  };

  /// \brief Read from JSON
  void from_json(Kinetics::DiffusionTransformation &trans, const jsonParser &json, const Structure &prim);

  template<>
  struct Printer<Kinetics::DiffusionTransformation> : public PrinterBase {

    typedef Kinetics::DiffusionTransformation Element;
    static const std::string element_name;

    Printer(int _indent_space = 6, char _delim = '\n', COORD_TYPE _mode = INTEGRAL) :
      PrinterBase(_indent_space, _delim, _mode) {}

    void print(const Element &element, std::ostream &out) const;
  };

}

#endif
