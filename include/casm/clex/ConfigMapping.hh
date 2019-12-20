#ifndef CASM_ConfigMapping
#define CASM_ConfigMapping

#include "casm/global/definitions.hh"
#include "casm/clex/Configuration.hh"
#include "casm/misc/CASM_math.hh"
#include "casm/crystallography/SimpleStrucMapCalculator.hh"
#include "casm/crystallography/StrucMapping.hh"
#include "casm/crystallography/BasicStructure.hh"

namespace CASM {
  namespace xtal {
    class Lattice;
    class Site;
    template<typename T>
    class BasicStructure;
    struct MappingNode;
    class SimpleStructure;
    class SimpleStrucMapCalculator;
    class StrucMapper;
  }
  using xtal::Lattice;
  using xtal::Site;
  using xtal::BasicStructure;
  using xtal::MappingNode;
  using xtal::SimpleStructure;
  using xtal::SimpleStrucMapCalculator;
  using xtal::StrucMapper;

  class Supercell;
  class PermuteIterator;
  class PrimClex;
  class Configuration;
  class ConfigDoF;
  class SupercellSymInfo;


  namespace Completer {
    class ImportOption;
  }


  /// \brief Reorders the permutation and compounds the spatial isometry (rotation + translation) of _node with that of _it
  MappingNode copy_apply(PermuteIterator const &_it, MappingNode const &_node, bool transform_cost_mat = true);

  /// \brief Initializes configdof from _child_struc, assuming it has been mapped exactly onto onto _scel
  /// This means that _child_struc has had its setting resolved using struc_mapper().calculator().resolve_setting()
  std::pair<ConfigDoF, std::set<std::string> > to_configdof(SimpleStructure const &_child_struc, Supercell const  &_scel);

  class PrimStrucMapCalculator : public SimpleStrucMapCalculator {
  public:
    PrimStrucMapCalculator(BasicStructure<Site> const &_prim,
                           std::vector<SymOp> const &symgroup = {},
                           SimpleStructure::SpeciesMode _species_mode = SimpleStructure::SpeciesMode::ATOM);

  private:
    /// \brief Make an exact copy of the calculator (including any initialized members)
    StrucMapCalculatorInterface *_clone() const override {
      return new PrimStrucMapCalculator(*this);
    }

    BasicStructure<Site> m_prim;

  };



  /// Data structure holding results of ConfigMapper algorithm
  struct ConfigMapperResult {
    enum class HintStatus { None, Derivative, Equivalent, Identical};
    struct Individual {
      Individual(Configuration _config,
                 SimpleStructure _resolved_struc,
                 std::set<std::string> _dof_managed,
                 HintStatus _hint_status = HintStatus::None) :
        config(std::move(_config)),
        resolved_struc(std::move(_resolved_struc)),
        dof_properties(std::move(_dof_managed)),
        hint_status(_hint_status) {}

      // list of properties that are handled by DoFs and are thus not considered properties
      Configuration config;
      SimpleStructure resolved_struc;
      std::set<std::string> dof_properties;
      HintStatus hint_status;
    };

    ConfigMapperResult() {}

    bool success() const {
      return !maps.empty();
    }

    Index n_optimal(double tol = TOL) const;

    /// Mapped structure, before applying lattice similarity and/or rotation to
    /// input structure.
    SimpleStructure structure;

    /// The configurations that the input structure mapped onto
    std::map<MappingNode, Individual> maps;


    /// Failure message if could not map to prim
    std::string fail_msg;

  };


  /// A class for mapping an arbitrary crystal structure as a configuration of a crystal template
  /// as described by a PrimClex.  ConfigMapper manages options for the mapping algorithm and mapping cost function
  /// It also caches some information about supercell lattices so that batch imports are more efficient
  ///
  /// \ingroup Configuration
  class ConfigMapper {
  public:

    ///\brief Construct and initialize a ConfigMapper
    ///\param _pclex the PrimClex that describes the crystal template
    ///
    ///\param _strain_weight
    ///\parblock
    ///          free parameter 'w' in the cost function: total_cost = w*lattice_deformation+(1-w)*basis_deformation
    ///          can vary between 0 (completely basis-focused) and 1 (completely lattice-focused)
    ///\endparblock
    ///
    ///\param _max_volume_change
    ///\parblock
    ///          constrains the search space by assuming a limit on allowed volume change
    ///          only taken into account when non-interstitial vacancies are allowed
    ///\endparblock
    ///
    ///\param _options
    ///\parblock
    ///          specify a combination of ConfigMapper::Options using bitwise OR: Ex. _options=ConfigMapper::rotate|ConfigMapper::strict
    ///          Options are:
    ///             'rotate': removes rigid rotation of the imported crystal, in a least-squares sense (i.e., yields a symmetric deformation tensor)
    ///             'robust': does not assume the imported structure might be ideal ('robust' is much slower for importing ideal structures, but if 'robust' is not
    ///                       set and a non-ideal structure is passed, this will be almost always be detected and robust methods will be used instead. Thus, 'robust'
    ///                       is slightly faster if imported Structures are *not* ideal)
    ///             'strict': prevents transformation into canonical form. Tries to preserve original orientation of imported structure if possible
    ///\endparblock
    ///
    ///\param _tol tolerance for mapping comparisons (default is _pclex.crystallography_tol())
    ConfigMapper(PrimClex const &_pclex,
                 double _strain_weight,
                 double _max_volume_change = 0.5,
                 int _options = 0, // this should actually be a bitwise-OR of StrucMapper::Options
                 double _tol = -1.);


    const PrimClex &primclex() const {
      return *m_pclex;
    }

    bool strict() const {
      return struc_mapper().options() && StrucMapper::strict;
    }

    void set_primclex(const PrimClex &_pclex) {
      m_pclex = &_pclex;
    }

    StrucMapper &struc_mapper() {
      return m_struc_mapper;
    }

    StrucMapper const &struc_mapper() const {
      return m_struc_mapper;
    }

    void add_allowed_lattices(std::vector<std::string> const &_lattice_names);

    void clear_allowed_lattices() {
      struc_mapper().clear_allowed_lattices();
    }

    //STEPS:
    //    0) [If Hint] Do SimpleStructure -> SimpleStructure(Config) mapping => HintMapping (Default, HintMapping.cost=inf())
    //    1) If HintMapping.cost>tol  Do SimpleStructure -> PrimClex mapping => ClexMapping (Default, ClexMapping.cost=inf())
    //    2) If HintMapping.cost<ClexMapping.cost, use HintMapping, else use ClexMapping => BestMapping
    //    3) Convert BestMapping to ConfigDoF
    //       [a] - BestMapping attributes that define ConfigDoF are mapped 'DoF', all others mapped 'property'
    //       [b] - 'property' attributes are subsumed into 'relaxation_properties' object
    //    4) Construct Configuration as ConfigDoF + relation_properties

    ///\brief imports structure specified by '_struc' into primclex()
    ///\param hint_ptr[in]
    ///\parblock
    ///                provides a suggestion for which Configuration _struc should map onto
    ///                The hint is used to reduce search times, but may be used in the future
    ///                in combination with Option 'strict' to force mapping onto a particular configuration
    ///                or be used to provide user reports of the form "Suggested mapping: 0.372; Optimal mapping: 0.002"
    ///\endparblock
    ///
    ConfigMapperResult import_structure(SimpleStructure const &_struc,
                                        Configuration const *hint_ptr = nullptr,
                                        std::vector<DoFKey> const &_hint_dofs = {"occ"}) const;

    ConfigMapperResult import_structure(SimpleStructure const &_struc,
                                        Index k,
                                        Configuration const *hint_ptr = nullptr,
                                        std::vector<DoFKey> const &_hint_dofs = {"occ"}) const;


  private:

    template <typename IterType, typename ScelType, typename OutputIter>
    void _to_configmaps(SimpleStructure const &child_struc,
                        IterType begin,
                        IterType end,
                        ScelType const &scel,
                        SupercellSymInfo const &sym_info,
                        OutputIter out) const;

    const PrimClex *m_pclex;
    ///Maps the supercell volume to a vector of Lattices with that volume
    StrucMapper m_struc_mapper;
  };


}

#endif
