// ---------------------------------------------------------------------
//
// Copyright (C) 1998 - 2016 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the deal.II distribution.
//
// ---------------------------------------------------------------------

#include <deal.II/base/memory_consumption.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_handler_policy.h>
#include <deal.II/dofs/dof_levels.h>
#include <deal.II/dofs/dof_faces.h>
#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>
#include <deal.II/grid/tria_levels.h>
#include <deal.II/grid/tria.h>
#include <deal.II/base/geometry_info.h>
#include <deal.II/fe/fe.h>
#include <deal.II/distributed/shared_tria.h>
#include <deal.II/distributed/tria.h>

#include <set>
#include <algorithm>

DEAL_II_NAMESPACE_OPEN


template <int dim, int spacedim>
const unsigned int DoFHandler<dim,spacedim>::dimension;

template <int dim, int spacedim>
const unsigned int DoFHandler<dim,spacedim>::space_dimension;

template <int dim, int spacedim>
const types::global_dof_index DoFHandler<dim,spacedim>::invalid_dof_index;

template <int dim, int spacedim>
const unsigned int DoFHandler<dim,spacedim>::default_fe_index;


// reference the invalid_dof_index variable explicitly to work around
// a bug in the icc8 compiler
namespace internal
{
  template <int dim, int spacedim>
  const types::global_dof_index *dummy ()
  {
    return &dealii::numbers::invalid_dof_index;
  }
}



namespace internal
{
  template <int dim, int spacedim>
  std::string policy_to_string(const dealii::internal::DoFHandler::Policy::PolicyBase<dim,spacedim> &policy)
  {
    std::string policy_name;
    if (dynamic_cast<const typename dealii::internal::DoFHandler::Policy::Sequential<dealii::DoFHandler<dim,spacedim> >*>(&policy))
      policy_name = "Policy::Sequential<";
    else if (dynamic_cast<const typename dealii::internal::DoFHandler::Policy::ParallelDistributed<dim,spacedim>*>(&policy))
      policy_name = "Policy::ParallelDistributed<";
    else if (dynamic_cast<const typename dealii::internal::DoFHandler::Policy::ParallelShared<dim,spacedim>*>(&policy))
      policy_name = "Policy::ParallelShared<";
    else
      AssertThrow(false, ExcNotImplemented());
    policy_name += Utilities::int_to_string(dim)+
                   ","+Utilities::int_to_string(spacedim)+">";
    return policy_name;
  }


  namespace DoFHandler
  {
    // access class
    // dealii::DoFHandler instead of
    // namespace internal::DoFHandler
    using dealii::DoFHandler;


    /**
     * A class with the same purpose as the similarly named class of the
     * Triangulation class. See there for more information.
     */
    struct Implementation
    {
      /**
       * Implement the function of same name in
       * the mother class.
       */
      template <int spacedim>
      static
      unsigned int
      max_couplings_between_dofs (const DoFHandler<1,spacedim> &dof_handler)
      {
        return std::min(static_cast<types::global_dof_index>(3*dof_handler.selected_fe->dofs_per_vertex +
                                                             2*dof_handler.selected_fe->dofs_per_line),
                        dof_handler.n_dofs());
      }



      template <int spacedim>
      static
      unsigned int
      max_couplings_between_dofs (const DoFHandler<2,spacedim> &dof_handler)
      {

        // get these numbers by drawing pictures
        // and counting...
        // example:
        //   |     |     |
        // --x-----x--x--X--
        //   |     |  |  |
        //   |     x--x--x
        //   |     |  |  |
        // --x--x--*--x--x--
        //   |  |  |     |
        //   x--x--x     |
        //   |  |  |     |
        // --X--x--x-----x--
        //   |     |     |
        // x = vertices connected with center vertex *;
        //   = total of 19
        // (the X vertices are connected with * if
        // the vertices adjacent to X are hanging
        // nodes)
        // count lines -> 28 (don't forget to count
        // mother and children separately!)
        types::global_dof_index max_couplings;
        switch (dof_handler.tria->max_adjacent_cells())
          {
          case 4:
            max_couplings=19*dof_handler.selected_fe->dofs_per_vertex +
                          28*dof_handler.selected_fe->dofs_per_line +
                          8*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 5:
            max_couplings=21*dof_handler.selected_fe->dofs_per_vertex +
                          31*dof_handler.selected_fe->dofs_per_line +
                          9*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 6:
            max_couplings=28*dof_handler.selected_fe->dofs_per_vertex +
                          42*dof_handler.selected_fe->dofs_per_line +
                          12*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 7:
            max_couplings=30*dof_handler.selected_fe->dofs_per_vertex +
                          45*dof_handler.selected_fe->dofs_per_line +
                          13*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 8:
            max_couplings=37*dof_handler.selected_fe->dofs_per_vertex +
                          56*dof_handler.selected_fe->dofs_per_line +
                          16*dof_handler.selected_fe->dofs_per_quad;
            break;

          // the following numbers are not based on actual counting but by
          // extrapolating the number sequences from the previous ones (for
          // example, for dofs_per_vertex, the sequence above is 19, 21, 28,
          // 30, 37, and is continued as follows):
          case 9:
            max_couplings=39*dof_handler.selected_fe->dofs_per_vertex +
                          59*dof_handler.selected_fe->dofs_per_line +
                          17*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 10:
            max_couplings=46*dof_handler.selected_fe->dofs_per_vertex +
                          70*dof_handler.selected_fe->dofs_per_line +
                          20*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 11:
            max_couplings=48*dof_handler.selected_fe->dofs_per_vertex +
                          73*dof_handler.selected_fe->dofs_per_line +
                          21*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 12:
            max_couplings=55*dof_handler.selected_fe->dofs_per_vertex +
                          84*dof_handler.selected_fe->dofs_per_line +
                          24*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 13:
            max_couplings=57*dof_handler.selected_fe->dofs_per_vertex +
                          87*dof_handler.selected_fe->dofs_per_line +
                          25*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 14:
            max_couplings=63*dof_handler.selected_fe->dofs_per_vertex +
                          98*dof_handler.selected_fe->dofs_per_line +
                          28*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 15:
            max_couplings=65*dof_handler.selected_fe->dofs_per_vertex +
                          103*dof_handler.selected_fe->dofs_per_line +
                          29*dof_handler.selected_fe->dofs_per_quad;
            break;
          case 16:
            max_couplings=72*dof_handler.selected_fe->dofs_per_vertex +
                          114*dof_handler.selected_fe->dofs_per_line +
                          32*dof_handler.selected_fe->dofs_per_quad;
            break;

          default:
            Assert (false, ExcNotImplemented());
            max_couplings=0;
          }
        return std::min(max_couplings,dof_handler.n_dofs());
      }


      template <int spacedim>
      static
      unsigned int
      max_couplings_between_dofs (const DoFHandler<3,spacedim> &dof_handler)
      {
//TODO:[?] Invent significantly better estimates than the ones in this function

        // doing the same thing here is a
        // rather complicated thing, compared
        // to the 2d case, since it is hard
        // to draw pictures with several
        // refined hexahedra :-) so I
        // presently only give a coarse
        // estimate for the case that at most
        // 8 hexes meet at each vertex
        //
        // can anyone give better estimate
        // here?
        const unsigned int max_adjacent_cells
          = dof_handler.tria->max_adjacent_cells();

        types::global_dof_index max_couplings;
        if (max_adjacent_cells <= 8)
          max_couplings=7*7*7*dof_handler.selected_fe->dofs_per_vertex +
                        7*6*7*3*dof_handler.selected_fe->dofs_per_line +
                        9*4*7*3*dof_handler.selected_fe->dofs_per_quad +
                        27*dof_handler.selected_fe->dofs_per_hex;
        else
          {
            Assert (false, ExcNotImplemented());
            max_couplings=0;
          }

        return std::min(max_couplings,dof_handler.n_dofs());
      }


      /**
       * Reserve enough space in the
       * <tt>levels[]</tt> objects to store the
       * numbers of the degrees of freedom
       * needed for the given element. The
       * given element is that one which
       * was selected when calling
       * @p distribute_dofs the last time.
       */
      template <int spacedim>
      static
      void reserve_space (DoFHandler<1,spacedim> &dof_handler)
      {
        dof_handler.vertex_dofs
        .resize(dof_handler.tria->n_vertices() *
                dof_handler.selected_fe->dofs_per_vertex,
                numbers::invalid_dof_index);

        for (unsigned int i=0; i<dof_handler.tria->n_levels(); ++i)
          {
            dof_handler.levels.emplace_back (new internal::DoFHandler::DoFLevel<1>);

            dof_handler.levels.back()->dof_object.dofs
            .resize (dof_handler.tria->n_raw_cells(i) *
                     dof_handler.selected_fe->dofs_per_line,
                     numbers::invalid_dof_index);

            dof_handler.levels.back()->cell_dof_indices_cache
            .resize (dof_handler.tria->n_raw_cells(i) *
                     dof_handler.selected_fe->dofs_per_cell,
                     numbers::invalid_dof_index);
          }
      }


      template <int spacedim>
      static
      void reserve_space (DoFHandler<2,spacedim> &dof_handler)
      {
        dof_handler.vertex_dofs
        .resize(dof_handler.tria->n_vertices() *
                dof_handler.selected_fe->dofs_per_vertex,
                numbers::invalid_dof_index);

        for (unsigned int i=0; i<dof_handler.tria->n_levels(); ++i)
          {
            dof_handler.levels.emplace_back (new internal::DoFHandler::DoFLevel<2>);

            dof_handler.levels.back()->dof_object.dofs
            .resize (dof_handler.tria->n_raw_cells(i) *
                     dof_handler.selected_fe->dofs_per_quad,
                     numbers::invalid_dof_index);

            dof_handler.levels.back()->cell_dof_indices_cache
            .resize (dof_handler.tria->n_raw_cells(i) *
                     dof_handler.selected_fe->dofs_per_cell,
                     numbers::invalid_dof_index);
          }

        dof_handler.faces.reset (new internal::DoFHandler::DoFFaces<2>);
        // avoid access to n_raw_lines when there are no cells
        if (dof_handler.tria->n_cells() > 0)
          {
            dof_handler.faces->lines.dofs
            .resize (dof_handler.tria->n_raw_lines() *
                     dof_handler.selected_fe->dofs_per_line,
                     numbers::invalid_dof_index);
          }
      }


      template <int spacedim>
      static
      void reserve_space (DoFHandler<3,spacedim> &dof_handler)
      {
        dof_handler.vertex_dofs
        .resize(dof_handler.tria->n_vertices() *
                dof_handler.selected_fe->dofs_per_vertex,
                numbers::invalid_dof_index);

        for (unsigned int i=0; i<dof_handler.tria->n_levels(); ++i)
          {
            dof_handler.levels.emplace_back (new internal::DoFHandler::DoFLevel<3>);

            dof_handler.levels.back()->dof_object.dofs
            .resize (dof_handler.tria->n_raw_cells(i) *
                     dof_handler.selected_fe->dofs_per_hex,
                     numbers::invalid_dof_index);

            dof_handler.levels.back()->cell_dof_indices_cache
            .resize (dof_handler.tria->n_raw_cells(i) *
                     dof_handler.selected_fe->dofs_per_cell,
                     numbers::invalid_dof_index);
          }
        dof_handler.faces.reset (new internal::DoFHandler::DoFFaces<3>);

        // avoid access to n_raw_lines when there are no cells
        if (dof_handler.tria->n_cells() > 0)
          {
            dof_handler.faces->lines.dofs
            .resize (dof_handler.tria->n_raw_lines() *
                     dof_handler.selected_fe->dofs_per_line,
                     numbers::invalid_dof_index);
            dof_handler.faces->quads.dofs
            .resize (dof_handler.tria->n_raw_quads() *
                     dof_handler.selected_fe->dofs_per_quad,
                     numbers::invalid_dof_index);
          }
      }

      template <int spacedim>
      static
      void reserve_space_mg (DoFHandler<1, spacedim> &dof_handler)
      {
        Assert (dof_handler.get_triangulation().n_levels () > 0, ExcMessage ("Invalid triangulation"));
        dof_handler.clear_mg_space ();

        const dealii::Triangulation<1, spacedim> &tria = dof_handler.get_triangulation();
        const unsigned int &dofs_per_line = dof_handler.get_fe ().dofs_per_line;
        const unsigned int &n_levels = tria.n_levels ();

        for (unsigned int i = 0; i < n_levels; ++i)
          {
            dof_handler.mg_levels.emplace_back (new internal::DoFHandler::DoFLevel<1>);
            dof_handler.mg_levels.back ()->dof_object.dofs = std::vector<types::global_dof_index> (tria.n_raw_lines (i) * dofs_per_line, numbers::invalid_dof_index);
          }

        const unsigned int &n_vertices = tria.n_vertices ();

        dof_handler.mg_vertex_dofs.resize (n_vertices);

        std::vector<unsigned int> max_level (n_vertices, 0);
        std::vector<unsigned int> min_level (n_vertices, n_levels);

        for (typename dealii::Triangulation<1, spacedim>::cell_iterator cell = tria.begin (); cell != tria.end (); ++cell)
          {
            const unsigned int level = cell->level ();

            for (unsigned int vertex = 0; vertex < GeometryInfo<1>::vertices_per_cell; ++vertex)
              {
                const unsigned int vertex_index = cell->vertex_index (vertex);

                if (min_level[vertex_index] > level)
                  min_level[vertex_index] = level;

                if (max_level[vertex_index] < level)
                  max_level[vertex_index] = level;
              }
          }

        for (unsigned int vertex = 0; vertex < n_vertices; ++vertex)
          if (tria.vertex_used (vertex))
            {
              Assert (min_level[vertex] < n_levels, ExcInternalError ());
              Assert (max_level[vertex] >= min_level[vertex], ExcInternalError ());
              dof_handler.mg_vertex_dofs[vertex].init (min_level[vertex], max_level[vertex], dof_handler.get_fe ().dofs_per_vertex);
            }

          else
            {
              Assert (min_level[vertex] == n_levels, ExcInternalError ());
              Assert (max_level[vertex] == 0, ExcInternalError ());
              dof_handler.mg_vertex_dofs[vertex].init (1, 0, 0);
            }
      }

      template <int spacedim>
      static
      void reserve_space_mg (DoFHandler<2, spacedim> &dof_handler)
      {
        Assert (dof_handler.get_triangulation().n_levels () > 0, ExcMessage ("Invalid triangulation"));
        dof_handler.clear_mg_space ();

        const dealii::FiniteElement<2, spacedim> &fe = dof_handler.get_fe ();
        const dealii::Triangulation<2, spacedim> &tria = dof_handler.get_triangulation();
        const unsigned int &n_levels = tria.n_levels ();

        for (unsigned int i = 0; i < n_levels; ++i)
          {
            dof_handler.mg_levels.emplace_back (new internal::DoFHandler::DoFLevel<2>);
            dof_handler.mg_levels.back ()->dof_object.dofs = std::vector<types::global_dof_index> (tria.n_raw_quads (i) * fe.dofs_per_quad, numbers::invalid_dof_index);
          }

        dof_handler.mg_faces.reset (new internal::DoFHandler::DoFFaces<2>);
        dof_handler.mg_faces->lines.dofs = std::vector<types::global_dof_index> (tria.n_raw_lines () * fe.dofs_per_line, numbers::invalid_dof_index);

        const unsigned int &n_vertices = tria.n_vertices ();

        dof_handler.mg_vertex_dofs.resize (n_vertices);

        std::vector<unsigned int> max_level (n_vertices, 0);
        std::vector<unsigned int> min_level (n_vertices, n_levels);

        for (typename dealii::Triangulation<2, spacedim>::cell_iterator cell = tria.begin (); cell != tria.end (); ++cell)
          {
            const unsigned int level = cell->level ();

            for (unsigned int vertex = 0; vertex < GeometryInfo<2>::vertices_per_cell; ++vertex)
              {
                const unsigned int vertex_index = cell->vertex_index (vertex);

                if (min_level[vertex_index] > level)
                  min_level[vertex_index] = level;

                if (max_level[vertex_index] < level)
                  max_level[vertex_index] = level;
              }
          }

        for (unsigned int vertex = 0; vertex < n_vertices; ++vertex)
          if (tria.vertex_used (vertex))
            {
              Assert (min_level[vertex] < n_levels, ExcInternalError ());
              Assert (max_level[vertex] >= min_level[vertex], ExcInternalError ());
              dof_handler.mg_vertex_dofs[vertex].init (min_level[vertex], max_level[vertex], fe.dofs_per_vertex);
            }

          else
            {
              Assert (min_level[vertex] == n_levels, ExcInternalError ());
              Assert (max_level[vertex] == 0, ExcInternalError ());
              dof_handler.mg_vertex_dofs[vertex].init (1, 0, 0);
            }
      }

      template <int spacedim>
      static
      void reserve_space_mg (DoFHandler<3, spacedim> &dof_handler)
      {
        Assert (dof_handler.get_triangulation().n_levels () > 0, ExcMessage ("Invalid triangulation"));
        dof_handler.clear_mg_space ();

        const dealii::FiniteElement<3, spacedim> &fe = dof_handler.get_fe ();
        const dealii::Triangulation<3, spacedim> &tria = dof_handler.get_triangulation();
        const unsigned int &n_levels = tria.n_levels ();

        for (unsigned int i = 0; i < n_levels; ++i)
          {
            dof_handler.mg_levels.emplace_back (new internal::DoFHandler::DoFLevel<3>);
            dof_handler.mg_levels.back ()->dof_object.dofs = std::vector<types::global_dof_index> (tria.n_raw_hexs (i) * fe.dofs_per_hex, numbers::invalid_dof_index);
          }

        dof_handler.mg_faces.reset (new internal::DoFHandler::DoFFaces<3>);
        dof_handler.mg_faces->lines.dofs = std::vector<types::global_dof_index> (tria.n_raw_lines () * fe.dofs_per_line, numbers::invalid_dof_index);
        dof_handler.mg_faces->quads.dofs = std::vector<types::global_dof_index> (tria.n_raw_quads () * fe.dofs_per_quad, numbers::invalid_dof_index);

        const unsigned int &n_vertices = tria.n_vertices ();

        dof_handler.mg_vertex_dofs.resize (n_vertices);

        std::vector<unsigned int> max_level (n_vertices, 0);
        std::vector<unsigned int> min_level (n_vertices, n_levels);

        for (typename dealii::Triangulation<3, spacedim>::cell_iterator cell = tria.begin (); cell != tria.end (); ++cell)
          {
            const unsigned int level = cell->level ();

            for (unsigned int vertex = 0; vertex < GeometryInfo<3>::vertices_per_cell; ++vertex)
              {
                const unsigned int vertex_index = cell->vertex_index (vertex);

                if (min_level[vertex_index] > level)
                  min_level[vertex_index] = level;

                if (max_level[vertex_index] < level)
                  max_level[vertex_index] = level;
              }
          }

        for (unsigned int vertex = 0; vertex < n_vertices; ++vertex)
          if (tria.vertex_used (vertex))
            {
              Assert (min_level[vertex] < n_levels, ExcInternalError ());
              Assert (max_level[vertex] >= min_level[vertex], ExcInternalError ());
              dof_handler.mg_vertex_dofs[vertex].init (min_level[vertex], max_level[vertex], fe.dofs_per_vertex);
            }

          else
            {
              Assert (min_level[vertex] == n_levels, ExcInternalError ());
              Assert (max_level[vertex] == 0, ExcInternalError ());
              dof_handler.mg_vertex_dofs[vertex].init (1, 0, 0);
            }
      }



      template <int spacedim>
      static
      types::global_dof_index
      get_dof_index (
        const DoFHandler<1, spacedim> &dof_handler,
        internal::DoFHandler::DoFLevel<1> &mg_level,
        internal::DoFHandler::DoFFaces<1> &,
        const unsigned int obj_index,
        const unsigned int fe_index,
        const unsigned int local_index,
        const int2type<1>)
      {
        return mg_level.dof_object.get_dof_index (dof_handler, obj_index, fe_index, local_index);
      }

      template <int spacedim>
      static
      types::global_dof_index
      get_dof_index (const DoFHandler<2, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<2> &, internal::DoFHandler::DoFFaces<2> &mg_faces, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const int2type<1>)
      {
        return mg_faces.lines.get_dof_index (dof_handler, obj_index, fe_index, local_index);
      }

      template <int spacedim>
      static
      types::global_dof_index
      get_dof_index (const DoFHandler<2, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<2> &mg_level, internal::DoFHandler::DoFFaces<2> &, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const int2type<2>)
      {
        return mg_level.dof_object.get_dof_index (dof_handler, obj_index, fe_index, local_index);
      }

      template <int spacedim>
      static
      types::global_dof_index
      get_dof_index (const DoFHandler<3, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<3> &, internal::DoFHandler::DoFFaces<3> &mg_faces, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const int2type<1>)
      {
        return mg_faces.lines.get_dof_index (dof_handler, obj_index, fe_index, local_index);
      }

      template <int spacedim>
      static
      types::global_dof_index
      get_dof_index (const DoFHandler<3, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<3> &, internal::DoFHandler::DoFFaces<3> &mg_faces, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const int2type<2>)
      {
        return mg_faces.quads.get_dof_index (dof_handler, obj_index, fe_index, local_index);
      }

      template <int spacedim>
      static
      types::global_dof_index
      get_dof_index (const DoFHandler<3, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<3> &mg_level, internal::DoFHandler::DoFFaces<3> &, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const int2type<3>)
      {
        return mg_level.dof_object.get_dof_index (dof_handler, obj_index, fe_index, local_index);
      }

      template <int spacedim>
      static
      void set_dof_index (const DoFHandler<1, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<1> &mg_level, internal::DoFHandler::DoFFaces<1> &, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const types::global_dof_index global_index, const int2type<1>)
      {
        mg_level.dof_object.set_dof_index (dof_handler, obj_index, fe_index, local_index, global_index);
      }

      template <int spacedim>
      static
      void set_dof_index (const DoFHandler<2, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<2> &, internal::DoFHandler::DoFFaces<2> &mg_faces, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const types::global_dof_index global_index, const int2type<1>)
      {
        mg_faces.lines.set_dof_index (dof_handler, obj_index, fe_index, local_index, global_index);
      }

      template <int spacedim>
      static
      void set_dof_index (const DoFHandler<2, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<2> &mg_level, internal::DoFHandler::DoFFaces<2> &, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const types::global_dof_index global_index, const int2type<2>)
      {
        mg_level.dof_object.set_dof_index (dof_handler, obj_index, fe_index, local_index, global_index);
      }

      template <int spacedim>
      static
      void set_dof_index (const DoFHandler<3, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<3> &, internal::DoFHandler::DoFFaces<3> &mg_faces, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const types::global_dof_index global_index, const int2type<1>)
      {
        mg_faces.lines.set_dof_index (dof_handler, obj_index, fe_index, local_index, global_index);
      }

      template <int spacedim>
      static
      void set_dof_index (const DoFHandler<3, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<3> &, internal::DoFHandler::DoFFaces<3> &mg_faces, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const types::global_dof_index global_index, const int2type<2>)
      {
        mg_faces.quads.set_dof_index (dof_handler, obj_index, fe_index, local_index, global_index);
      }

      template <int spacedim>
      static
      void set_dof_index (const DoFHandler<3, spacedim> &dof_handler, internal::DoFHandler::DoFLevel<3> &mg_level, internal::DoFHandler::DoFFaces<3> &, const unsigned int obj_index, const unsigned int fe_index, const unsigned int local_index, const types::global_dof_index global_index, const int2type<3>)
      {
        mg_level.dof_object.set_dof_index (dof_handler, obj_index, fe_index, local_index, global_index);
      }
    };
  }
}



template <int dim, int spacedim>
DoFHandler<dim,spacedim>::DoFHandler (const Triangulation<dim,spacedim> &tria)
  :
  tria(&tria, typeid(*this).name()),
  selected_fe(nullptr, typeid(*this).name()),
  faces(nullptr),
  mg_faces (nullptr)
{
  // decide whether we need a sequential or a parallel distributed policy
  if (dynamic_cast<const parallel::shared::Triangulation< dim, spacedim>*>
      (&tria)
      != nullptr)
    policy.reset (new internal::DoFHandler::Policy::ParallelShared<dim,spacedim>(*this));
  else if (dynamic_cast<const parallel::distributed::Triangulation< dim, spacedim >*>
           (&tria)
           == nullptr)
    policy.reset (new internal::DoFHandler::Policy::Sequential<DoFHandler<dim,spacedim> >(*this));
  else
    policy.reset (new internal::DoFHandler::Policy::ParallelDistributed<dim,spacedim>(*this));
}


template <int dim, int spacedim>
DoFHandler<dim,spacedim>::DoFHandler ()
  :
  tria(nullptr, typeid(*this).name()),
  selected_fe(nullptr, typeid(*this).name())
{}


template <int dim, int spacedim>
DoFHandler<dim,spacedim>::~DoFHandler ()
{
  // release allocated memory
  clear ();

  // also release the policy. this needs to happen before the
  // current object disappears because the policy objects
  // store references to the DoFhandler object they work on
  policy.reset ();
}


template <int dim, int spacedim>
void
DoFHandler<dim,spacedim>::
initialize(const Triangulation<dim,spacedim> &t,
           const FiniteElement<dim,spacedim> &fe)
{
  tria = &t;
  faces = nullptr;
  number_cache.n_global_dofs = 0;

  // decide whether we need a
  // sequential or a parallel
  // distributed policy
  if (dynamic_cast<const parallel::shared::Triangulation< dim, spacedim>*>
      (&t)
      != nullptr)
    policy.reset (new internal::DoFHandler::Policy::ParallelShared<dim,spacedim>(*this));
  else if (dynamic_cast<const parallel::distributed::Triangulation< dim, spacedim >*>
           (&t)
           == nullptr)
    policy.reset (new internal::DoFHandler::Policy::Sequential<DoFHandler<dim,spacedim> >(*this));
  else
    policy.reset (new internal::DoFHandler::Policy::ParallelDistributed<dim,spacedim>(*this));

  distribute_dofs(fe);
}



/*------------------------ Cell iterator functions ------------------------*/

template <int dim, int spacedim>
typename DoFHandler<dim,spacedim>::cell_iterator
DoFHandler<dim,spacedim>::begin (const unsigned int level) const
{
  typename Triangulation<dim,spacedim>::cell_iterator cell = this->get_triangulation().begin(level);
  if (cell == this->get_triangulation().end(level))
    return end(level);
  return cell_iterator (*cell, this);
}



template <int dim, int spacedim>
typename DoFHandler<dim,spacedim>::active_cell_iterator
DoFHandler<dim,spacedim>::begin_active (const unsigned int level) const
{
  // level is checked in begin
  cell_iterator i = begin (level);
  if (i.state() != IteratorState::valid)
    return i;
  while (i->has_children())
    if ((++i).state() != IteratorState::valid)
      return i;
  return i;
}



template <int dim, int spacedim>
typename DoFHandler<dim,spacedim>::cell_iterator
DoFHandler<dim,spacedim>::end () const
{
  return cell_iterator (&this->get_triangulation(),
                        -1,
                        -1,
                        this);
}


template <int dim, int spacedim>
typename DoFHandler<dim,spacedim>::cell_iterator
DoFHandler<dim,spacedim>::end (const unsigned int level) const
{
  typename Triangulation<dim,spacedim>::cell_iterator cell = this->get_triangulation().end(level);
  if (cell.state() != IteratorState::valid)
    return end();
  return cell_iterator (*cell, this);
}


template <int dim, int spacedim>
typename DoFHandler<dim, spacedim>::active_cell_iterator
DoFHandler<dim, spacedim>::end_active (const unsigned int level) const
{
  typename Triangulation<dim,spacedim>::cell_iterator cell = this->get_triangulation().end_active(level);
  if (cell.state() != IteratorState::valid)
    return active_cell_iterator(end());
  return active_cell_iterator (*cell, this);
}



template <int dim, int spacedim>
typename DoFHandler<dim, spacedim>::level_cell_iterator
DoFHandler<dim, spacedim>::begin_mg (const unsigned int level) const
{
  // Assert(this->has_level_dofs(), ExcMessage("You can only iterate over mg "
  //     "levels if mg dofs got distributed."));
  typename Triangulation<dim,spacedim>::cell_iterator cell = this->get_triangulation().begin(level);
  if (cell == this->get_triangulation().end(level))
    return end_mg(level);
  return level_cell_iterator (*cell, this);
}


template <int dim, int spacedim>
typename DoFHandler<dim, spacedim>::level_cell_iterator
DoFHandler<dim, spacedim>::end_mg (const unsigned int level) const
{
  // Assert(this->has_level_dofs(), ExcMessage("You can only iterate over mg "
  //     "levels if mg dofs got distributed."));
  typename Triangulation<dim,spacedim>::cell_iterator cell = this->get_triangulation().end(level);
  if (cell.state() != IteratorState::valid)
    return end();
  return level_cell_iterator (*cell, this);
}


template <int dim, int spacedim>
typename DoFHandler<dim, spacedim>::level_cell_iterator
DoFHandler<dim, spacedim>::end_mg () const
{
  return level_cell_iterator (&this->get_triangulation(), -1, -1, this);
}



template <int dim, int spacedim>
IteratorRange<typename DoFHandler<dim, spacedim>::cell_iterator>
DoFHandler<dim, spacedim>::cell_iterators () const
{
  return
    IteratorRange<typename DoFHandler<dim, spacedim>::cell_iterator>
    (begin(), end());
}


template <int dim, int spacedim>
IteratorRange<typename DoFHandler<dim, spacedim>::active_cell_iterator>
DoFHandler<dim, spacedim>::active_cell_iterators () const
{
  return
    IteratorRange<typename DoFHandler<dim, spacedim>::active_cell_iterator>
    (begin_active(), end());
}



template <int dim, int spacedim>
IteratorRange<typename DoFHandler<dim, spacedim>::level_cell_iterator>
DoFHandler<dim, spacedim>::mg_cell_iterators () const
{
  return
    IteratorRange<typename DoFHandler<dim, spacedim>::level_cell_iterator>
    (begin_mg(), end_mg());
}




template <int dim, int spacedim>
IteratorRange<typename DoFHandler<dim, spacedim>::cell_iterator>
DoFHandler<dim, spacedim>::cell_iterators_on_level (const unsigned int level) const
{
  return
    IteratorRange<typename DoFHandler<dim, spacedim>::cell_iterator>
    (begin(level), end(level));
}



template <int dim, int spacedim>
IteratorRange<typename DoFHandler<dim, spacedim>::active_cell_iterator>
DoFHandler<dim, spacedim>::active_cell_iterators_on_level (const unsigned int level) const
{
  return
    IteratorRange<typename DoFHandler<dim, spacedim>::active_cell_iterator>
    (begin_active(level), end_active(level));
}



template <int dim, int spacedim>
IteratorRange<typename DoFHandler<dim, spacedim>::level_cell_iterator>
DoFHandler<dim, spacedim>::mg_cell_iterators_on_level (const unsigned int level) const
{
  return
    IteratorRange<typename DoFHandler<dim, spacedim>::level_cell_iterator>
    (begin_mg(level), end_mg(level));
}



//---------------------------------------------------------------------------



template <>
types::global_dof_index DoFHandler<1>::n_boundary_dofs () const
{
  return 2*get_fe().dofs_per_vertex;
}



template <>
template <typename number>
types::global_dof_index DoFHandler<1>::n_boundary_dofs (const std::map<types::boundary_id, const Function<1,number>*> &boundary_ids) const
{
  // check that only boundary
  // indicators 0 and 1 are allowed
  // in 1d
  for (typename std::map<types::boundary_id, const Function<1,number>*>::const_iterator i=boundary_ids.begin();
       i!=boundary_ids.end(); ++i)
    Assert ((i->first == 0) || (i->first == 1),
            ExcInvalidBoundaryIndicator());

  return boundary_ids.size()*get_fe().dofs_per_vertex;
}



template <>
types::global_dof_index DoFHandler<1>::n_boundary_dofs (const std::set<types::boundary_id> &boundary_ids) const
{
  // check that only boundary
  // indicators 0 and 1 are allowed
  // in 1d
  for (std::set<types::boundary_id>::const_iterator i=boundary_ids.begin();
       i!=boundary_ids.end(); ++i)
    Assert ((*i == 0) || (*i == 1),
            ExcInvalidBoundaryIndicator());

  return boundary_ids.size()*get_fe().dofs_per_vertex;
}


template <>
types::global_dof_index DoFHandler<1,2>::n_boundary_dofs () const
{
  return 2*get_fe().dofs_per_vertex;
}



template <>
template <typename number>
types::global_dof_index DoFHandler<1,2>::n_boundary_dofs (const std::map<types::boundary_id, const Function<2,number>*> &boundary_ids) const
{
  // check that only boundary
  // indicators 0 and 1 are allowed
  // in 1d
  for (typename std::map<types::boundary_id, const Function<2,number>*>::const_iterator i=boundary_ids.begin();
       i!=boundary_ids.end(); ++i)
    Assert ((i->first == 0) || (i->first == 1),
            ExcInvalidBoundaryIndicator());

  return boundary_ids.size()*get_fe().dofs_per_vertex;
}



template <>
types::global_dof_index DoFHandler<1,2>::n_boundary_dofs (const std::set<types::boundary_id> &boundary_ids) const
{
  // check that only boundary
  // indicators 0 and 1 are allowed
  // in 1d
  for (std::set<types::boundary_id>::const_iterator i=boundary_ids.begin();
       i!=boundary_ids.end(); ++i)
    Assert ((*i == 0) || (*i == 1),
            ExcInvalidBoundaryIndicator());

  return boundary_ids.size()*get_fe().dofs_per_vertex;
}



template <int dim, int spacedim>
types::global_dof_index DoFHandler<dim,spacedim>::n_boundary_dofs () const
{
  std::set<int> boundary_dofs;

  const unsigned int dofs_per_face = get_fe().dofs_per_face;
  std::vector<types::global_dof_index> dofs_on_face(dofs_per_face);

  // loop over all faces of all cells
  // and see whether they are at a
  // boundary. note (i) that we visit
  // interior faces twice (which we
  // don't care about) but exterior
  // faces only once as is
  // appropriate, and (ii) that we
  // need not take special care of
  // single lines (using
  // @p{cell->has_boundary_lines}),
  // since we do not support
  // boundaries of dimension dim-2,
  // and so every boundary line is
  // also part of a boundary face.
  active_cell_iterator cell = begin_active (),
                       endc = end();
  for (; cell!=endc; ++cell)
    for (unsigned int f=0; f<GeometryInfo<dim>::faces_per_cell; ++f)
      if (cell->at_boundary(f))
        {
          cell->face(f)->get_dof_indices (dofs_on_face);
          for (unsigned int i=0; i<dofs_per_face; ++i)
            boundary_dofs.insert(dofs_on_face[i]);
        }

  return boundary_dofs.size();
}



template <int dim, int spacedim>
types::global_dof_index
DoFHandler<dim,spacedim>::n_boundary_dofs (const std::set<types::boundary_id> &boundary_ids) const
{
  Assert (boundary_ids.find (numbers::internal_face_boundary_id) == boundary_ids.end(),
          ExcInvalidBoundaryIndicator());

  std::set<types::global_dof_index> boundary_dofs;

  const unsigned int dofs_per_face = get_fe().dofs_per_face;
  std::vector<types::global_dof_index> dofs_on_face(dofs_per_face);

  // same as in the previous
  // function, but with a different
  // check for the boundary indicator
  active_cell_iterator cell = begin_active (),
                       endc = end();
  for (; cell!=endc; ++cell)
    for (unsigned int f=0; f<GeometryInfo<dim>::faces_per_cell; ++f)
      if (cell->at_boundary(f)
          &&
          (std::find (boundary_ids.begin(),
                      boundary_ids.end(),
                      cell->face(f)->boundary_id()) !=
           boundary_ids.end()))
        {
          cell->face(f)->get_dof_indices (dofs_on_face);
          for (unsigned int i=0; i<dofs_per_face; ++i)
            boundary_dofs.insert(dofs_on_face[i]);
        }

  return boundary_dofs.size();
}



template <int dim, int spacedim>
std::size_t
DoFHandler<dim,spacedim>::memory_consumption () const
{
  std::size_t mem = (MemoryConsumption::memory_consumption (tria) +
                     MemoryConsumption::memory_consumption (selected_fe) +
                     MemoryConsumption::memory_consumption (block_info_object) +
                     MemoryConsumption::memory_consumption (levels) +
                     MemoryConsumption::memory_consumption (*faces) +
                     MemoryConsumption::memory_consumption (faces) +
                     sizeof (number_cache) +
                     MemoryConsumption::memory_consumption (mg_number_cache) +
                     MemoryConsumption::memory_consumption (vertex_dofs));
  for (unsigned int i=0; i<levels.size(); ++i)
    mem += MemoryConsumption::memory_consumption (*levels[i]);

  for (unsigned int level = 0; level < mg_levels.size (); ++level)
    mem += mg_levels[level]->memory_consumption ();

  if (mg_faces != nullptr)
    mem += MemoryConsumption::memory_consumption (*mg_faces);

  for (unsigned int i = 0; i < mg_vertex_dofs.size (); ++i)
    mem += sizeof (MGVertexDoFs) + (1 + mg_vertex_dofs[i].get_finest_level () - mg_vertex_dofs[i].get_coarsest_level ()) * sizeof (types::global_dof_index);

  return mem;
}



template <int dim, int spacedim>
void DoFHandler<dim,spacedim>::distribute_dofs (const FiniteElement<dim,spacedim> &ff)
{
  selected_fe = &ff;

  // delete all levels and set them
  // up newly. note that we still
  // have to allocate space for all
  // degrees of freedom on this mesh
  // (including ghost and cells that
  // are entirely stored on different
  // processors), though we may not
  // assign numbers to some of them
  // (i.e. they will remain at
  // invalid_dof_index). We need to
  // allocate the space because we
  // will want to be able to query
  // the dof_indices on each cell,
  // and simply be told that we don't
  // know them on some cell (i.e. get
  // back invalid_dof_index)
  clear_space ();
  internal::DoFHandler::Implementation::reserve_space (*this);

  // hand things off to the policy
  number_cache = policy->distribute_dofs ();

  // initialize the block info object
  // only if this is a sequential
  // triangulation. it doesn't work
  // correctly yet if it is parallel
  if (dynamic_cast<const parallel::distributed::Triangulation<dim,spacedim>*>(&*tria) == nullptr)
    block_info_object.initialize(*this, false, true);
}


template <int dim, int spacedim>
void DoFHandler<dim, spacedim>::distribute_mg_dofs (const FiniteElement<dim, spacedim> &fe)
{
  (void)fe;
  Assert(levels.size()>0, ExcMessage("Distribute active DoFs using distribute_dofs() before calling distribute_mg_dofs()."));

  const FiniteElement<dim, spacedim> *old_fe = selected_fe;
  (void)old_fe;
  Assert(old_fe == &fe, ExcMessage("You are required to use the same FE for level and active DoFs!") );

  Assert(((tria->get_mesh_smoothing() & Triangulation<dim, spacedim>::limit_level_difference_at_vertices)
          != Triangulation<dim, spacedim>::none),
         ExcMessage("The mesh smoothing requirement 'limit_level_difference_at_vertices' has to be set for using multigrid!"));

  clear_mg_space();

  internal::DoFHandler::Implementation::reserve_space_mg (*this);
  mg_number_cache = policy->distribute_mg_dofs ();

  // initialize the block info object
  // only if this is a sequential
  // triangulation. it doesn't work
  // correctly yet if it is parallel
  if (dynamic_cast<const parallel::distributed::Triangulation<dim,spacedim>*>(&*tria) == nullptr)
    block_info_object.initialize (*this, true, false);
}



template <int dim, int spacedim>
void DoFHandler<dim, spacedim>::reserve_space ()
{
  //TODO: move this to distribute_mg_dofs and remove function
}



template <int dim, int spacedim>
void DoFHandler<dim, spacedim>::clear_mg_space ()
{
  mg_levels.clear ();
  mg_faces.reset ();

  std::vector<MGVertexDoFs> tmp;

  std::swap (mg_vertex_dofs, tmp);

  mg_number_cache.clear();
}


template <int dim, int spacedim>
void DoFHandler<dim,spacedim>::initialize_local_block_info ()
{
  block_info_object.initialize_local(*this);
}



template <int dim, int spacedim>
void DoFHandler<dim,spacedim>::clear ()
{
  // release lock to old fe
  selected_fe = nullptr;

  // release memory
  clear_space ();
  clear_mg_space ();
}



template <int dim, int spacedim>
void
DoFHandler<dim,spacedim>::renumber_dofs (const std::vector<types::global_dof_index> &new_numbers)
{
  Assert(levels.size()>0, ExcMessage("You need to distribute DoFs before you can renumber them."));

  Assert (new_numbers.size() == n_locally_owned_dofs(),
          ExcRenumberingIncomplete());

#ifdef DEBUG
  // assert that the new indices are
  // consecutively numbered if we are
  // working on a single
  // processor. this doesn't need to
  // hold in the case of a parallel
  // mesh since we map the interval
  // [0...n_dofs()) into itself but
  // only globally, not on each
  // processor
  if (n_locally_owned_dofs() == n_dofs())
    {
      std::vector<types::global_dof_index> tmp(new_numbers);
      std::sort (tmp.begin(), tmp.end());
      std::vector<types::global_dof_index>::const_iterator p = tmp.begin();
      types::global_dof_index i = 0;
      for (; p!=tmp.end(); ++p, ++i)
        Assert (*p == i, ExcNewNumbersNotConsecutive(i));
    }
  else
    for (types::global_dof_index i=0; i<new_numbers.size(); ++i)
      Assert (new_numbers[i] < n_dofs(),
              ExcMessage ("New DoF index is not less than the total number of dofs."));
#endif

  number_cache = policy->renumber_dofs (new_numbers);
}


template <int dim, int spacedim>
void
DoFHandler<dim,spacedim>::renumber_dofs (const unsigned int                          level,
                                         const std::vector<types::global_dof_index> &new_numbers)
{
  Assert(mg_levels.size()>0 && levels.size()>0,
         ExcMessage("You need to distribute active and level DoFs before you can renumber level DoFs."));
  AssertIndexRange(level, levels.size());
  Assert (new_numbers.size() == n_dofs(level), ExcRenumberingIncomplete());

  mg_number_cache[level] = policy->renumber_mg_dofs (level, new_numbers);
}



template <int dim, int spacedim>
unsigned int
DoFHandler<dim,spacedim>::max_couplings_between_dofs () const
{
  return internal::DoFHandler::Implementation::max_couplings_between_dofs (*this);
}



template <int dim, int spacedim>
unsigned int
DoFHandler<dim,spacedim>::max_couplings_between_boundary_dofs () const
{
  switch (dim)
    {
    case 1:
      return get_fe().dofs_per_vertex;
    case 2:
      return (3*get_fe().dofs_per_vertex +
              2*get_fe().dofs_per_line);
    case 3:
      // we need to take refinement of
      // one boundary face into
      // consideration here; in fact,
      // this function returns what
      // #max_coupling_between_dofs<2>
      // returns
      //
      // we assume here, that only four
      // faces meet at the boundary;
      // this assumption is not
      // justified and needs to be
      // fixed some time. fortunately,
      // omitting it for now does no
      // harm since the matrix will cry
      // foul if its requirements are
      // not satisfied
      return (19*get_fe().dofs_per_vertex +
              28*get_fe().dofs_per_line +
              8*get_fe().dofs_per_quad);
    default:
      Assert (false, ExcNotImplemented());
      return numbers::invalid_unsigned_int;
    }
}



template <int dim, int spacedim>
void DoFHandler<dim,spacedim>::clear_space ()
{
  levels.clear ();
  faces.reset ();

  std::vector<types::global_dof_index> tmp;
  std::swap (vertex_dofs, tmp);

  number_cache.clear ();
}



template <int dim, int spacedim>
template <int structdim>
types::global_dof_index
DoFHandler<dim, spacedim>::get_dof_index (const unsigned int obj_level,
                                          const unsigned int obj_index,
                                          const unsigned int fe_index,
                                          const unsigned int local_index) const
{
  return internal::DoFHandler::Implementation::get_dof_index (*this, *this->mg_levels[obj_level],
                                                              *this->mg_faces, obj_index,
                                                              fe_index, local_index,
                                                              internal::int2type<structdim> ());
}



template <int dim, int spacedim>
template <int structdim>
void DoFHandler<dim, spacedim>::set_dof_index (const unsigned int obj_level,
                                               const unsigned int obj_index,
                                               const unsigned int fe_index,
                                               const unsigned int local_index,
                                               const types::global_dof_index global_index) const
{
  internal::DoFHandler::Implementation::set_dof_index (*this,
                                                       *this->mg_levels[obj_level],
                                                       *this->mg_faces,
                                                       obj_index,
                                                       fe_index,
                                                       local_index,
                                                       global_index,
                                                       internal::int2type<structdim> ());
}



template <int dim, int spacedim>
DoFHandler<dim, spacedim>::MGVertexDoFs::MGVertexDoFs ()
  :
  coarsest_level (numbers::invalid_unsigned_int),
  finest_level (0)
{}



template <int dim, int spacedim>
void DoFHandler<dim, spacedim>::MGVertexDoFs::init (const unsigned int cl,
                                                    const unsigned int fl,
                                                    const unsigned int n_dofs_per_vertex)
{
  coarsest_level  = cl;
  finest_level    = fl;
  dofs_per_vertex = n_dofs_per_vertex;

  if (coarsest_level <= finest_level)
    {
      const unsigned int n_levels = finest_level - coarsest_level + 1;
      const unsigned int n_indices = n_levels * dofs_per_vertex;

      indices.reset (new types::global_dof_index[n_indices]);
      std::fill (indices.get(), indices.get()+n_indices,
                 numbers::invalid_dof_index);
    }
  else
    indices.reset ();
}



template <int dim, int spacedim>
unsigned int DoFHandler<dim, spacedim>::MGVertexDoFs::get_coarsest_level () const
{
  return coarsest_level;
}



template <int dim, int spacedim>
unsigned int DoFHandler<dim, spacedim>::MGVertexDoFs::get_finest_level () const
{
  return finest_level;
}


/*-------------- Explicit Instantiations -------------------------------*/
#include "dof_handler.inst"


DEAL_II_NAMESPACE_CLOSE
