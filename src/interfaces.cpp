#include "RBGL.h"

extern "C" {
#include <R.h>
#include <Rmath.h>
#include <Rdefines.h>
}

/* need a template with C++ linkage for BFS */
/* adapted from Siek's bfs-example.cpp */

template < typename TimeMap > class bfs_time_visitor:public default_bfs_visitor {
  typedef typename property_traits < TimeMap >::value_type T;
public:
  bfs_time_visitor(TimeMap tmap, T & t):m_timemap(tmap), m_time(t) { }
  template < typename Vertex, typename Graph >
    void discover_vertex(Vertex u, const Graph & g) const
  {
    put(m_timemap, u, m_time++);
  }
  TimeMap m_timemap;
  T & m_time;
};

template < typename TimeMap > class dfs_time_visitor:public default_dfs_visitor {
  typedef typename property_traits < TimeMap >::value_type T;
public:
  dfs_time_visitor(TimeMap dmap, TimeMap fmap, T & t)
:  m_dtimemap(dmap), m_ftimemap(fmap), m_time(t) {
  }
  template < typename Vertex, typename Graph >
    void discover_vertex(Vertex u, const Graph & g) const
  {
    put(m_dtimemap, u, m_time++);
  }
  template < typename Vertex, typename Graph >
    void finish_vertex(Vertex u, const Graph & g) const
  {
    put(m_ftimemap, u, m_time++);
  }
  TimeMap m_dtimemap;
  TimeMap m_ftimemap;
  T & m_time;
};

extern "C" 
{
	SEXP BGL_tsort_D(SEXP num_verts_in, SEXP num_edges_in, SEXP R_edges_in)
	{
	// tsortbCG -- for bioConductor graph objects (only graphNEL at present)
	  
	setupGraphTypes
	setTraits( Graph_dd )
	setUnweightedDoubleEdges( Graph_dd )
	
	typedef property_map<Graph_dd, vertex_color_t>::type Color;
	graph_traits<Graph_dd>::vertex_iterator viter, viter_end;
	 
	typedef list<Vertex> tsOrder;
	tsOrder tsord;
	SEXP tsout;
	
	PROTECT(tsout = NEW_NUMERIC(INTEGER(num_verts_in)[0]));
	
	try {
	      topological_sort(g, std::front_inserter(tsord));
	
	      int j = 0;
	      for (tsOrder::iterator i = tsord.begin();
	             i != tsord.end(); ++i)
	             {
	             REAL(tsout)[j] = (double) *i;
	             j++;
	             }
	      }
	catch ( not_a_dag )
	      {
	      Rprintf("not a dag, returning zeroes\n");
	      for (int j = 0 ; j < INTEGER(num_verts_in)[0]; j++)
	          REAL(tsout)[j] = 0.0;
	      }
	UNPROTECT(1);

	return(tsout);
	} // end BGL_tsort_D
 

	SEXP BGL_KMST_D( SEXP num_verts_in, SEXP num_edges_in, 
	    SEXP R_edges_in, SEXP R_weights_in)
	{
	using namespace boost;
	
	Rprintf("warning: directed graph supplied; directions ignored.\n");
	
	setupGraphTypes
	setTraits( Graph_dd )
	setWeightedDoubleEdges( Graph_dd )
	
	std::vector < Edge > spanning_tree;
	
	kruskal_minimum_spanning_tree(g, std::back_inserter(spanning_tree));
	
	SEXP ansList;
	PROTECT(ansList = allocVector(VECSXP,2));
	SEXP ans;
	PROTECT(ans = allocMatrix(INTSXP,2,spanning_tree.size()));
	SEXP answt;
	PROTECT(answt = allocMatrix(REALSXP,1,spanning_tree.size()));
	int k = 0;
	int j = 0;
	
	for (std::vector < Edge >::iterator ei = spanning_tree.begin();
		ei != spanning_tree.end(); ++ei) 
		{
	   		INTEGER(ans)[k] = source(*ei,g);
		   	INTEGER(ans)[k+1] = target(*ei,g);
	   		REAL(answt)[j] = weight[*ei];
	   		k = k + 2;
	   		j = j + 1;
	  	}
	
	SET_VECTOR_ELT(ansList,0,ans);
	SET_VECTOR_ELT(ansList,1,answt);
	UNPROTECT(3);
	return(ansList);
	} //end BGL_KMST_D


	SEXP BGL_BFS_D(SEXP num_verts_in, SEXP num_edges_in, 
		SEXP R_edges_in, SEXP R_weights_in, SEXP init_ind)
	{
	using namespace boost;
	
	setupGraphTypes
	setTraits( Graph_dd )
	setWeightedDoubleEdges( Graph_dd )
	
	typedef graph_traits < Graph_dd >::vertices_size_type size_type;
	
	const int N = INTEGER(num_verts_in)[0];
	// Typedefs
	typedef size_type* Iiter;
	
	// discover time properties
	std::vector < size_type > dtime(num_vertices(g));
	
	size_type time = 0;
	bfs_time_visitor < size_type * >vis(&dtime[0], time);
	breadth_first_search(g, vertex((int)INTEGER(init_ind)[0], g), visitor(vis));
	
	
	// use std::sort to order the vertices by their discover time
	std::vector < size_type > discover_order(N);
	integer_range < size_type > r(0, N);
	std::copy(r.begin(), r.end(), discover_order.begin());
	std::sort(discover_order.begin(), discover_order.end(),
	            indirect_cmp < Iiter, std::less < size_type > >(&dtime[0]));
	
	SEXP disc;
	PROTECT(disc = allocVector(INTSXP,N));
	
	int i;
	  for (i = 0; i < N; ++i)
	    {
	    INTEGER(disc)[i] = discover_order[i];
	    }
	
	UNPROTECT(1);
	return(disc);
	}


	SEXP BGL_dfs_D(SEXP num_verts_in, SEXP num_edges_in, SEXP R_edges_in,
	    SEXP R_weights_in)
	{
	using namespace boost;
	
	setupGraphTypes
	setTraits( Graph_dd )
	setWeightedDoubleEdges( Graph_dd )
	
	typedef graph_traits < Graph_dd >::vertices_size_type size_type;
	
	const int N = INTEGER(num_verts_in)[0];
	  // Typedefs
	typedef size_type* Iiter;
	
	  // discover time and finish time properties
	std::vector < size_type > dtime(num_vertices(g));
	std::vector < size_type > ftime(num_vertices(g));
	size_type t = 0;
	dfs_time_visitor < size_type * >vis(&dtime[0], &ftime[0], t);
	
	depth_first_search(g, visitor(vis));
	
	  // use std::sort to order the vertices by their discover time
	std::vector < size_type > discover_order(N);
	integer_range < size_type > r(0, N);
	std::copy(r.begin(), r.end(), discover_order.begin());
	std::sort(discover_order.begin(), discover_order.end(),
	            indirect_cmp < Iiter, std::less < size_type > >(&dtime[0]));
	std::vector < size_type > finish_order(N);
	std::copy(r.begin(), r.end(), finish_order.begin());
	std::sort(finish_order.begin(), finish_order.end(),
	            indirect_cmp < Iiter, std::less < size_type > >(&ftime[0]));
	
	SEXP ansList;
	PROTECT(ansList = allocVector(VECSXP,2));
	SEXP disc;
	PROTECT(disc = allocVector(INTSXP,N));
	SEXP fin;
	PROTECT(fin = allocVector(INTSXP,N));
	
	int i;
	for (i = 0; i < N; ++i)
	    {
	    INTEGER(disc)[i] = discover_order[i];
	    INTEGER(fin)[i] = finish_order[i];
	    }
	
	SET_VECTOR_ELT(ansList,0,disc);
	SET_VECTOR_ELT(ansList,1,fin);
	UNPROTECT(3);
	return(ansList);
	}

	SEXP BGL_dijkstra_shortest_paths_D (SEXP num_verts_in, 
		SEXP num_edges_in, SEXP R_edges_in,
		SEXP R_weights_in, SEXP init_ind)
	{
	using namespace boost;
	
	setupGraphTypes
	setTraits( Graph_dd )
	setWeightedDoubleEdges( Graph_dd )
	
	int N = num_vertices(g);
	std::vector<Vertex> p(N);
	std::vector<double> d(N);
	
	dijkstra_shortest_paths(g, vertex((int)INTEGER(init_ind)[0], g),
	         predecessor_map(&p[0]).distance_map(&d[0]));
	
	SEXP dists, pens, ansList;
	PROTECT(dists = allocVector(REALSXP,N));
	PROTECT(pens = allocVector(INTSXP,N));
	graph_traits < Graph_dd >::vertex_iterator vi, vend;
	for (tie(vi, vend) = vertices(g); vi != vend; ++vi) {
	    REAL(dists)[*vi] = d[*vi];
	    INTEGER(pens)[*vi] = p[*vi];
	  }
	PROTECT(ansList = allocVector(VECSXP,2));
	SET_VECTOR_ELT(ansList,0,dists);
	SET_VECTOR_ELT(ansList,1,pens);
	
	UNPROTECT(3);
	return(ansList);
	}
		
}