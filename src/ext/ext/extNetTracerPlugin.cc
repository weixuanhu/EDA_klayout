
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include "extNetTracerIO.h"
#include "extNetTracerDialog.h"
#include "extNetTracerConfig.h"

#include "layConverters.h"
#include "layCellView.h"

#include "gsiDecl.h"

namespace tl
{
  /**
   *  @brief A specialization of the XMLConverter that is used to serialize the connection info
   */

  template <>
  struct XMLStdConverter<ext::NetTracerConnectionInfo>
  {
    std::string to_string (const ext::NetTracerConnectionInfo &v) const
    {
      return v.to_string ();
    }

    void from_string (const std::string &s, ext::NetTracerConnectionInfo &v) const
    {
      tl::Extractor ex (s.c_str ());
      v.parse (ex);
    }
  };

  /**
   *  @brief A specialization of the XMLConverter that is used to serialize the symbol info
   */

  template <>
  struct XMLStdConverter<ext::NetTracerSymbolInfo>
  {
    std::string to_string (const ext::NetTracerSymbolInfo &v) const
    {
      return v.to_string ();
    }

    void from_string (const std::string &s, ext::NetTracerSymbolInfo &v) const
    {
      tl::Extractor ex (s.c_str ());
      v.parse (ex);
    }
  };
}

namespace ext
{

extern std::string net_tracer_component_name;

// -----------------------------------------------------------------------------------
//  NetTracerPlugin definition and implementation

class NetTracerTechnologyEditorProvider
  : public lay::TechnologyEditorProvider
{
public:
  virtual lay::TechnologyComponentEditor *create_editor (QWidget *parent) const
  {
    return new NetTracerTechComponentEditor (parent);
  }
};

static tl::RegisteredClass<lay::TechnologyEditorProvider> editor_decl (new NetTracerTechnologyEditorProvider (), 13000, "NetTracerPlugin");

class NetTracerPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_nt_window_mode, "fit-net"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_window_dim, "1.0"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_max_shapes_highlighted, "10000"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_color, lay::ColorConverter ().to_string (QColor ())));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_cycle_colors_enabled, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_cycle_colors, "255,0,0 0,255,0 0,0,255 255,255,0 255,0,255 0,255,255 160,80,255 255,160,0"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_line_width, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_vertex_size, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_halo, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_dither_pattern, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_intensity, "50"));
  }

  virtual std::vector<std::pair <std::string, lay::ConfigPage *> > config_pages (QWidget *parent) const
  {
    std::vector<std::pair <std::string, lay::ConfigPage *> > pages;
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Other Tools|Net Tracer")), new NetTracerConfigPage (parent)));
    return pages;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    //  TODO: where should that go?
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::MenuEntry ("net_trace_group", "tools_menu.end"));
    menu_entries.push_back (lay::MenuEntry ("ext::net_trace", "net_trace", "tools_menu.end", tl::to_string (QObject::tr ("Trace Net"))));
  }

  virtual lay::Plugin *create_plugin (db::Manager * /*manager*/, lay::PluginRoot *root, lay::LayoutView *view) const
  {
    return new NetTracerDialog (root, view);
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new NetTracerPluginDeclaration (), 13000, "NetTracerPlugin");

class NetTracerTechnologyComponentProvider
  : public db::TechnologyComponentProvider
{
public:
  NetTracerTechnologyComponentProvider ()
    : db::TechnologyComponentProvider ()
  {
    //  .. nothing yet ..
  }

  virtual db::TechnologyComponent *create_component () const
  {
    return new NetTracerTechnologyComponent ();
  }

  virtual tl::XMLElementBase *xml_element () const
  {
    return new db::TechnologyComponentXMLElement<NetTracerTechnologyComponent> (net_tracer_component_name,
      tl::make_member ((NetTracerTechnologyComponent::const_iterator (NetTracerTechnologyComponent::*) () const) &NetTracerTechnologyComponent::begin, (NetTracerTechnologyComponent::const_iterator (NetTracerTechnologyComponent::*) () const) &NetTracerTechnologyComponent::end, &NetTracerTechnologyComponent::add, "connection") +
      tl::make_member ((NetTracerTechnologyComponent::const_symbol_iterator (NetTracerTechnologyComponent::*) () const) &NetTracerTechnologyComponent::begin_symbols, (NetTracerTechnologyComponent::const_symbol_iterator (NetTracerTechnologyComponent::*) () const) &NetTracerTechnologyComponent::end_symbols, &NetTracerTechnologyComponent::add_symbol, "symbols")
    );
  }
};

static tl::RegisteredClass<db::TechnologyComponentProvider> tc_decl (new NetTracerTechnologyComponentProvider (), 13000, "NetTracerPlugin");

}

// -----------------------------------------------------------------------------------
//  GSI binding

namespace gsi
{

static void def_connection2 (ext::NetTracerTechnologyComponent *tech, const std::string &la, const std::string &lb)
{
  ext::NetTracerLayerExpressionInfo la_info = ext::NetTracerLayerExpressionInfo::compile (la);
  ext::NetTracerLayerExpressionInfo lb_info = ext::NetTracerLayerExpressionInfo::compile (lb);
  tech->add (ext::NetTracerConnectionInfo (la_info, lb_info));
}

static void def_connection3 (ext::NetTracerTechnologyComponent *tech, const std::string &la, const std::string &via, const std::string &lb)
{
  ext::NetTracerLayerExpressionInfo la_info = ext::NetTracerLayerExpressionInfo::compile (la);
  ext::NetTracerLayerExpressionInfo via_info = ext::NetTracerLayerExpressionInfo::compile (via);
  ext::NetTracerLayerExpressionInfo lb_info = ext::NetTracerLayerExpressionInfo::compile (lb);
  tech->add (ext::NetTracerConnectionInfo (la_info, via_info, lb_info));
}

static void def_symbol (ext::NetTracerTechnologyComponent *tech, const std::string &name, const std::string &expr)
{
  tech->add_symbol (ext::NetTracerSymbolInfo (db::LayerProperties (name), expr));
}

gsi::Class<db::TechnologyComponent> &decl_layTechnologyComponent ();

gsi::Class<ext::NetTracerTechnologyComponent> decl_NetTracerTechnology (decl_layTechnologyComponent (), "lay", "NetTracerTechnology",
  gsi::method_ext ("connection", &def_connection2, gsi::arg("a"), gsi::arg("b"),
    "@brief Defines a connection between two materials\n"
    "See the class description for details about this method."
  ) +
  gsi::method_ext ("connection", &def_connection3, gsi::arg("a"), gsi::arg("via"), gsi::arg("b"),
    "@brief Defines a connection between materials through a via\n"
    "See the class description for details about this method."
  ) +
  gsi::method_ext ("symbol", &def_symbol, gsi::arg("name"), gsi::arg("expr"),
    "@brief Defines a symbol for use in the material expressions.\n"
    "Defines a sub-expression to be used in further symbols or material expressions. "
    "For the detailed notation of the expression see the description of the net tracer feature."
  ),
  "@brief A technology description for the net tracer\n"
  "\n"
  "This object represents the technology description for the net tracer (represented by the \\NetTracer class).\n"
  "A technology description basically consists of connection declarations.\n"
  "A connection is given by either two or three expressions describing two conductive materials.\n"
  "With two expressions, the connection describes a transition from one material to another one.\n"
  "With three expressions, the connection describes a transition from one material to another through a "
  "connection (a \"via\").\n"
  "\n"
  "The conductive material is derived from original layers either directly or through "
  "boolean expressions. These expressions can include symbols which are defined through the "
  "\\symbol method.\n"
  "\n"
  "For details about the expressions see the description of the net tracer feature.\n"
  "\n"
  "This class has been introduced in version 0.25.\n"
);

static void trace1 (ext::NetTracer *net_tracer, const ext::NetTracerTechnologyComponent &tech, const db::Layout &layout, const db::Cell &cell, const db::Point &start_point, unsigned int start_layer)
{
  ext::NetTracerData tracer_data = tech.get_tracer_data (layout);
  net_tracer->trace (layout, cell, start_point, start_layer, tracer_data);
}

static void trace2 (ext::NetTracer *net_tracer, const ext::NetTracerTechnologyComponent &tech, const db::Layout &layout, const db::Cell &cell, const db::Point &start_point, unsigned int start_layer, const db::Point &stop_point, unsigned int stop_layer)
{
  ext::NetTracerData tracer_data = tech.get_tracer_data (layout);
  net_tracer->trace (layout, cell, start_point, start_layer, stop_point, stop_layer, tracer_data);
}

static ext::NetTracerData get_tracer_data_from_cv (const lay::CellViewRef &cv)
{
  const db::Technology *tech = cv->technology ();
  tl_assert (tech != 0);

  const ext::NetTracerTechnologyComponent *tech_component = dynamic_cast <const ext::NetTracerTechnologyComponent *> (tech->component_by_name (ext::net_tracer_component_name));
  tl_assert (tech_component != 0);

  return tech_component->get_tracer_data (cv->layout ());
}

static void trace1_cv (ext::NetTracer *net_tracer, const lay::CellViewRef &cv, const db::Point &start_point, unsigned int start_layer)
{
  ext::NetTracerData tracer_data = get_tracer_data_from_cv (cv);
  net_tracer->trace (cv->layout (), *cv.cell (), start_point, start_layer, tracer_data);
}

static void trace2_cv (ext::NetTracer *net_tracer, const lay::CellViewRef &cv, const db::Point &start_point, unsigned int start_layer, const db::Point &stop_point, unsigned int stop_layer)
{
  ext::NetTracerData tracer_data = get_tracer_data_from_cv (cv);
  net_tracer->trace (cv->layout (), *cv.cell (), start_point, start_layer, stop_point, stop_layer, tracer_data);
}

static ext::NetTracerData get_tracer_data_from_tech (const std::string &tech_name, const db::Layout &layout)
{
  const db::Technology *tech = db::Technologies::instance ()->technology_by_name (tech_name);
  tl_assert (tech != 0);

  const ext::NetTracerTechnologyComponent *tech_component = dynamic_cast <const ext::NetTracerTechnologyComponent *> (tech->component_by_name (ext::net_tracer_component_name));
  tl_assert (tech_component != 0);

  return tech_component->get_tracer_data (layout);
}

static void trace1_tn (ext::NetTracer *net_tracer, const std::string &tech, const db::Layout &layout, const db::Cell &cell, const db::Point &start_point, unsigned int start_layer)
{
  ext::NetTracerData tracer_data = get_tracer_data_from_tech (tech, layout);
  net_tracer->trace (layout, cell, start_point, start_layer, tracer_data);
}

static void trace2_tn (ext::NetTracer *net_tracer, const std::string &tech, const db::Layout &layout, const db::Cell &cell, const db::Point &start_point, unsigned int start_layer, const db::Point &stop_point, unsigned int stop_layer)
{
  ext::NetTracerData tracer_data = get_tracer_data_from_tech (tech, layout);
  net_tracer->trace (layout, cell, start_point, start_layer, stop_point, stop_layer, tracer_data);
}

gsi::Class<ext::NetTracerShape> decl_NetElement ("lay", "NetElement",
  gsi::method ("trans", &ext::NetTracerShape::trans,
    "@brief Gets the transformation to apply for rendering the shape in the original top cell\n"
    "See the class description for more details about this attribute."
  ) +
  gsi::method ("shape", (const db::Shape &(ext::NetTracerShape::*) () const) &ext::NetTracerShape::shape,
    "@brief Gets the shape that makes up this net element\n"
    "See the class description for more details about this attribute."
  ) +
#if 0
  gsi::method ("is_valid?", &ext::NetTracerShape::is_valid,
    "@brief Gets a value indicating whether the shape is valid\n"
    "Currently this flag is not used."
  ) +
  gsi::method ("is_pseudo?", &ext::NetTracerShape::is_pseudo,
    "@brief Gets a value indicating whether the shape is a pseudo shape\n"
    "Currently this flag is not used."
  ) +
#endif
  gsi::method ("cell_index", &ext::NetTracerShape::cell_index,
    "@brief Gets the index of the cell the shape is inside"
  ) +
  gsi::method ("layer", &ext::NetTracerShape::layer,
    "@brief Gets the index of the layer the shape is on"
  ) +
  gsi::method ("bbox", &ext::NetTracerShape::bbox,
    "@brief Delivers the bounding box of the shape as seen from the original top cell"
  ),
  "@brief A net element for the \\NetTracer net tracing facility\n"
  "\n"
  "This object represents a piece of a net extracted by the net tracer. "
  "See the description of \\NetTracer for more details about the net tracer feature.\n"
  "\n"
  "The NetTracer object represents one shape of the net. The shape can be an original shape or a shape derived in a boolean operation. "
  "In the first case, the shape refers to a shape within a cell or a subcell of the original top cell. In the latter case, the shape "
  "is a synthesized one and outside the original layout hierarchy.\n"
  "\n"
  "In any case, the \\shape method will deliver the shape and \\trans the transformation of the shape into the original top cell. "
  "To obtain a flat representation of the net, the shapes need to be transformed by this transformation.\n"
  "\n"
  "\\layer will give the layer the shape is located at, \\cell_index will denote the cell that containes the shape.\n"
  "\n"
  "This class has been introduced in version 0.25.\n"
);

gsi::Class<ext::NetTracer> decl_NetTracer ("lay", "NetTracer",
  gsi::method_ext ("trace", &trace1, gsi::arg ("tech"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("start_point"), gsi::arg ("start_layer"),
    "@brief Runs a net extraction\n"
    "\n"
    "This method runs an extraction with the given parameters.\n"
    "To make the extraction successful, a shape must be present at the given start point on the start layer. "
    "The start layer must be a valid layer mentioned within the technology specification.\n"
    "\n"
    "This version runs a single extraction - i.e. it will extract all elements connected to the given seed point. "
    "A path extraction version is provided as well which will extract one (the presumably shortest) path between two "
    "points.\n"
    "\n"
    "@param tech The technology definition\n"
    "@param layout The layout on which to run the extraction\n"
    "@param cell The cell on which to run the extraction (child cells will be included)\n"
    "@param start_point The start point from which to start extraction of the net\n"
    "@param start_layer The layer from which to start extraction\n"
  ) +
  gsi::method_ext ("trace", &trace2, gsi::arg ("tech"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("start_point"), gsi::arg ("start_layer"), gsi::arg ("stop_point"), gsi::arg ("stop_layer"),
    "@brief Runs a path extraction\n"
    "\n"
    "This method runs an path extraction with the given parameters.\n"
    "To make the extraction successful, a shape must be present at the given start point on the start layer and "
    "at the given stop point at the given stop layer. "
    "The start and stop layers must be a valid layers mentioned within the technology specification.\n"
    "\n"
    "This version runs a path extraction and will deliver elements forming one path leading from the start to the end point.\n"
    "\n"
    "@param tech The technology definition\n"
    "@param layout The layout on which to run the extraction\n"
    "@param cell The cell on which to run the extraction (child cells will be included)\n"
    "@param start_point The start point from which to start extraction of the net\n"
    "@param start_layer The layer from which to start extraction\n"
    "@param stop_point The stop point at which to stop extraction of the net\n"
    "@param stop_layer The layer at which to stop extraction\n"
  ) +
  gsi::method_ext ("trace", &trace1_tn, gsi::arg ("tech"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("start_point"), gsi::arg ("start_layer"),
    "@brief Runs a net extraction taking a predefined technology\n"
    "This method behaves identical as the version with a technology object, except that it will look for a technology "
    "with the given name to obtain the extraction setup."
  ) +
  gsi::method_ext ("trace", &trace2_tn, gsi::arg ("tech"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("start_point"), gsi::arg ("start_layer"), gsi::arg ("stop_point"), gsi::arg ("stop_layer"),
    "@brief Runs a path extraction taking a predefined technology\n"
    "This method behaves identical as the version with a technology object, except that it will look for a technology "
    "with the given name to obtain the extraction setup."
  ) +
  gsi::method_ext ("trace", &trace1_cv, gsi::arg ("cellview"), gsi::arg ("start_point"), gsi::arg ("start_layer"),
    "@brief Runs a net extraction from a cell view\n"
    "This method behaves identical as the version with a technology, layout and cell object, except that it will take these "
    "from the cellview specified."
  ) +
  gsi::method_ext ("trace", &trace2_cv, gsi::arg ("cellview"), gsi::arg ("start_point"), gsi::arg ("start_layer"), gsi::arg ("stop_point"), gsi::arg ("stop_layer"),
    "@brief Runs a path extraction from a cell view\n"
    "This method behaves identical as the version with a technology, layout and cell object, except that it will take these "
    "from the cellview specified."
  ) +
  gsi::iterator ("each_element", &ext::NetTracer::begin, &ext::NetTracer::end,
    "@brief Iterates over the elements found during extraction\n"
    "The elements are available only after the extraction has been performed."
  ) +
  gsi::method ("num_elements", &ext::NetTracer::size,
    "@brief Returns the number of elements found during extraction\n"
    "This attribute is useful only after the extraction has been performed."
  ) +
  gsi::method ("clear", &ext::NetTracer::clear,
    "@brief Clears the data from the last extraction\n"
  ) +
  gsi::method ("name", &ext::NetTracer::name,
    "@brief Returns the name of the net found during extraction\n"
    "The net name is extracted from labels found during the extraction. "
    "This attribute is useful only after the extraction has been performed."
  ) +
  gsi::method ("incomplete?", &ext::NetTracer::incomplete,
    "@brief Returns a value indicating whether the net is incomplete\n"
    "A net may be incomplete if the extraction has been stopped by the user for example. "
    "This attribute is useful only after the extraction has been performed."
  ),
  "@brief The net tracer feature\n"
  "\n"
  "The net tracer class provides an interface to the net tracer feature. It is accompanied by the \\NetElement and \\NetTracerTechnology classes. "
  "The latter will provide the technology definition for the net tracer while the \\NetElement objects represent a piece of the net "
  "after it has been extracted.\n"
  "\n"
  "The technology definition is optional. The net tracer can be used with a predefined technology as well. The basic "
  "scheme of using the net tracer is to instantiate a net tracer object and run the extraction through the \\NetTracer#trace "
  "method. After this method was executed successfully, the resulting net can be obtained from the net tracer object by "
  "iterating over the \\NetElement objects of the net tracer.\n"
  "\n"
  "Here is some sample code:\n"
  "\n"
  "@code\n"
  "ly = RBA::CellView::active.layout\n"
  "\n"
  "tracer = RBA::NetTracer::new\n"
  "\n"
  "tech = RBA::NetTracerTechnology::new\n"
  "tech.connection(\"1/0\", \"2/0\", \"3/0\")\n"
  "\n"
  "tracer.trace(tech, ly, ly.top_cell, RBA::Point::new(7000, 1500), ly.find_layer(1, 0))\n"
  "\n"
  "tracer.each_element do |e|\n"
  "  puts e.shape.polygon.transformed(e.trans)\n"
  "end\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.25."
);

}

