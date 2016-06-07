
#include "PlanarGraph.h"

CGraphFace CPlanarGraph::m_faceTmp;
std::vector<CGraphFace> CPlanarGraph::m_faces;

CPlanarGraph::CPlanarGraph()
{
	m_pickedNodeIndex = -1;
	m_numOfTypes = 1;
}

void CPlanarGraph::ClearGraph()
{
	m_nodes.clear();
	m_edges.clear();
}

void CPlanarGraph::PrintGraph()
{
	std::cout << "The graph contains " << m_nodes.size() << " nodes and " << m_edges.size() << " edges!\n";
	std::cout << "Visited flags: ";
	for ( int i=0; i<int(m_nodes.size()); i++ )
	{
		std::cout << m_nodes[i].GetFlagVisited() << " ";
	}
	std::cout << std::endl;
	std::cout << "Node types: ";
	for ( int i=0; i<int(m_nodes.size()); i++ )
	{
		std::cout << m_nodes[i].GetType() << " ";
	}
	std::cout << std::endl;
}

bool CPlanarGraph::LoadGraphFromXML(const char* fileName, bool flagDetectFaces /* = true */, bool flagIgnoreIndiv /* = true */)
{
	// Clear the current graph...
	ClearGraph();

	TiXmlDocument doc;
	bool loadFlag = doc.LoadFile(fileName);
	if ( loadFlag == false )
	{
		std::cout << "Failed to load graph from " << fileName << "!\n";
		return loadFlag;
	}
	//doc.Print();

	TiXmlNode* xmlRoot = 0;
	xmlRoot = doc.RootElement();
	assert( xmlRoot );

	TiXmlNode* xmlNode = xmlRoot->FirstChild();
	while ( xmlNode != 0 )
	{
		if ( strcmp(xmlNode->Value(), "Node") == 0 )
		{
			// Parse a node...
			CGraphNode graphNode;
			float px, py;
			int rx = xmlNode->ToElement()->QueryFloatAttribute("px", &px);
			int ry = xmlNode->ToElement()->QueryFloatAttribute("py", &py);
			if ( rx != TIXML_SUCCESS || ry != TIXML_SUCCESS )
			{
				graphNode.RandomlyInitPos();
			}
			else
			{
				graphNode.SetPos(px, py);
			}
			int type;
			int r = xmlNode->ToElement()->QueryIntAttribute("type", &type);
			if ( r == TIXML_SUCCESS )
			{
				graphNode.SetType(type);
			}
			int boundary;
			int rb = xmlNode->ToElement()->QueryIntAttribute("boundary", &boundary);
			if ( rb == TIXML_SUCCESS )
			{
				graphNode.SetBoundaryType(boundary);
			}
			int fixed;
			int rf = xmlNode->ToElement()->QueryIntAttribute("fix", &fixed);
			if ( rf == TIXML_SUCCESS && fixed != 0 )
			{
				graphNode.SetFlagFixed(true);
			}
			const char* str = xmlNode->ToElement()->Attribute("name");
			if ( *str != '\0' )
			{
				graphNode.SetName(str);
			}
			AddGraphNode(graphNode);
		}
		else if ( strcmp(xmlNode->Value(), "Edge") == 0 )
		{
			// Parse an edge...
			int idx0 = -1;
			int idx1 = -1;
			int r0 = xmlNode->ToElement()->QueryIntAttribute("node0", &idx0);
			int r1 = xmlNode->ToElement()->QueryIntAttribute("node1", &idx1);
			if ( r0 != TIXML_SUCCESS )
			{
				const char* str = xmlNode->ToElement()->Attribute("name0");
				idx0 = FindNodeAccordingToName(str);
			}
			if ( r1 != TIXML_SUCCESS )
			{
				const char* str = xmlNode->ToElement()->Attribute("name1");
				idx1 = FindNodeAccordingToName(str);
			}
			if ( idx0 >= 0 && idx1 >= 0 )
			{
				CGraphEdge graphEdge(idx0, idx1);
				AddGraphEdge(graphEdge);
			}
		}

		// Move to the next sibling...
		xmlNode = xmlNode->NextSibling();
	}

	SetNodeNeighbors();
	if ( flagIgnoreIndiv == true )
	{
		RemoveIndividualNodes();
	}
	if ( flagDetectFaces == false )
	{
		return loadFlag;
	}
	//PrintGraph();

	// Step 1: Detect faces of the planar graph...
	DetectFaces();
	return loadFlag;
}

bool CPlanarGraph::SaveGraphAsXML(const char* fileName)
{
	const char* str =	"\t<?xml version=\"1.0\" standalone=\"yes\" ?>\n"
		"<!-- graph input data -->\n"
		"<Graph>\n"
		"</Graph>\n";
	TiXmlDocument doc;
	doc.Parse(str);
	TiXmlNode* root = doc.RootElement();
	// Dump nodes...
	for ( int i=0; i<int(m_nodes.size()); i++ )
	{
		TiXmlElement nodeElem("Node");
		nodeElem.SetAttribute("name", m_nodes[i].GetName().c_str());
		std::ostringstream oss0;
		std::ostringstream oss1;
		oss0 << m_nodes[i].GetPos()[0];
		oss1 << m_nodes[i].GetPos()[1];
		nodeElem.SetAttribute("px", oss0.str().c_str());
		nodeElem.SetAttribute("py", oss1.str().c_str());
		std::ostringstream ossType;
		ossType << m_nodes[i].GetType();
		nodeElem.SetAttribute("type", ossType.str().c_str());
		std::ostringstream ossFixed;
		ossFixed << m_nodes[i].GetFlagFixed();
		nodeElem.SetAttribute("fix", ossFixed.str().c_str());
		if ( m_nodes[i].GetBoundaryType() != 0 )
		{
			nodeElem.SetAttribute("boundary", m_nodes[i].GetBoundaryType());
		}
		root->InsertEndChild(nodeElem);
	}
	// Dump edges...
	for ( int i=0; i<int(m_edges.size()); i++ )
	{
		TiXmlElement edgeElem("Edge");
		edgeElem.SetAttribute("node0", m_edges[i].GetIdx0());
		edgeElem.SetAttribute("node1", m_edges[i].GetIdx1());
		root->InsertEndChild(edgeElem);
	}

	bool saveFlag = doc.SaveFile(fileName);
	return saveFlag;
}

bool CPlanarGraph::AddGraphEdge(const CGraphEdge& edge)
{
	if ( CheckDuplicatedEdge(edge) == true )
	{
		return false;
	}
	m_edges.push_back(edge);
	return true;
}

bool CPlanarGraph::CheckDuplicatedEdge(const CGraphEdge& edge)
{
	int idx10 = edge.GetIdx0();
	int idx11 = edge.GetIdx1();
	for ( int i=0; i<GetNumOfEdges(); i++ )
	{
		CGraphEdge& edge0 = GetEdge(i);
		int idx00 = edge0.GetIdx0();
		int idx01 = edge0.GetIdx1();
		if ( (idx00 == idx10 && idx01 == idx11) || (idx00 == idx11 && idx01 == idx10) )
		{
			return true;
		}
	}
	return false;
}

void CPlanarGraph::AddGraphNodes(int numOfNodes, int parent /* = -1 */)
{
	int current = parent;
	for ( int i=0; i<numOfNodes; i++ )
	{
		CGraphNode node;
        std::ostringstream os;
        os << GetNumOfNodes();
        std::string name = "node" + os.str();
		node.SetName(name);
		AddGraphNode(node);
		int curId = GetNumOfNodes() - 1;
		if ( current >= 0 )
		{
			AddGraphEdge(CGraphEdge(current, curId));
		}
		current = curId;
	}
}

void CPlanarGraph::SetNodeNeighbors()
{
	for ( int i=0; i<int(m_nodes.size()); i++ )
	{
		m_nodes[i].ClearNeighbors();
	}
	for ( int i=0; i<int(m_edges.size()); i++ )
	{
		int idx0 = m_edges[i].GetIdx0();
		int idx1 = m_edges[i].GetIdx1();
		m_nodes[idx0].AddNeighbor(idx1);
		m_nodes[idx1].AddNeighbor(idx0);
	}
}

void CPlanarGraph::PickClosestNode(float px, float py)
{
	float distSqrMin = std::numeric_limits<float>::max();
	v2f p;
	p[0] = px;
	p[1] = py;
	for ( int i=0; i<int(m_nodes.size()); i++ )
	{
		v2f pi = m_nodes[i].GetPos();
		float distSqrTmp = mag2(pi - p);
		if ( distSqrTmp < distSqrMin )
		{
			distSqrMin = distSqrTmp;
			m_pickedNodeIndex = i;
		}
	}
	std::cout << "Have picked the " << m_pickedNodeIndex << "th node centered at " << GetNodePos(m_pickedNodeIndex) << "!\n";
}

void CPlanarGraph::MovePickedNode(float dx, float dy)
{
	if ( m_pickedNodeIndex < 0 )
	{
		return;
	}
	v2f pos = m_nodes[m_pickedNodeIndex].GetPos();
	pos[0] += dx;
	pos[1] += dy;
	m_nodes[m_pickedNodeIndex].SetPos(pos);
}

void CPlanarGraph::RandomInitGraph()
{
	ClearGraph();

	int minSkeletonSize = 5;
	int additionalSkeletonSize = Random2(5);
	int mainSkeletonSize = minSkeletonSize + additionalSkeletonSize;
	AddGraphNodes(mainSkeletonSize);

	// Add blue key branch: don't let this happen in the main room
	int blueLoop = Random2(minSkeletonSize-1) + 1;

	// add red key branch: don't let this happen in the main room
	int redLoop = Random2(minSkeletonSize-1) + 1;

	int blueSectionLength = 4 + Random2(4);
	int redSectionLength = 4 + Random2(4);

	if ( blueLoop < mainSkeletonSize )
	{
		AddGraphNodes(blueSectionLength, blueLoop);
		if ( CoinFlip() )
		{
			int idx0 = GetNumOfNodes() - 1;
			int idx1 = Random2(minSkeletonSize);
			AddGraphEdge(CGraphEdge(idx0, idx1));
		}
	}

	if ( redLoop < mainSkeletonSize )
	{
		AddGraphNodes(redSectionLength, redLoop);
		if ( CoinFlip() )
		{
			int idx0 = GetNumOfNodes() - 1;
			int idx1 = Random2(minSkeletonSize);
			AddGraphEdge(CGraphEdge(idx0, idx1));
		}
	}

	// Add decoration
	int numStubs = Random2(5);
	for ( int i=0; i<numStubs; i++ )
	{
		int offset = Random2(mainSkeletonSize);
		AddGraphNodes(1+Random2(2), offset);
	}
}

void CPlanarGraph::RandomInitPositions()
{
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		GetNode(i).RandomlyInitPos();
	}
}

void CPlanarGraph::RandomInitTypes()
{
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		int type = int(rand() / float(RAND_MAX) * m_numOfTypes);
		GetNode(i).SetType(type);
	}
}

void CPlanarGraph::DetectFaces()
{
	// Based on the example in (http://www.boost.org/doc/libs/1_47_0/libs/graph/example/planar_face_traversal.cpp)
	int numOfNodes = GetNumOfNodes();
	Graph g(numOfNodes);

	int numOfEdges = GetNumOfEdges();
	for ( int i=0; i<numOfEdges; i++ )
	{
		int idx0 = GetEdge(i).GetIdx0();
		int idx1 = GetEdge(i).GetIdx1();
		add_edge(idx0, idx1, g);
	}

	// Initialize the interior edge index
	property_map<Graph, edge_index_t>::type e_index = get(edge_index, g);
	graph_traits<Graph>::edges_size_type edge_count = 0;
	graph_traits<Graph>::edge_iterator ei, ei_end;
	for ( boost::tuples::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei )
	{
		put(e_index, *ei, edge_count++);
	}

	typedef std::vector< graph_traits<Graph>::edge_descriptor > vec_t;
	std::vector<vec_t> embedding(num_vertices(g));
#if 0
	// Test for planarity - we know it is planar, we just want to 
	// compute the planar embedding as a side-effect
	if ( boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
		boyer_myrvold_params::embedding = &embedding[0]	) )
	{
		std::cout << "Input graph is planar" << std::endl;
	}
	else
	{
		std::cout << "Input graph is not planar" << std::endl;
	}
#else
	// Compute the planar embedding based on node positions...
	VertexIterator vi, vi_end;
	for ( boost::tie(vi, vi_end) = boost::vertices(g); vi != vi_end; ++vi )
	{
		OutEdgeIterator ei, ei_end;
		std::vector<EdgeDescriptor> adjacentEdges;
		for ( boost::tie(ei, ei_end) = boost::out_edges(*vi, g); ei != ei_end; ++ei )
		{
			VertexDescriptor v1 = boost::source(*ei, g);
			VertexDescriptor v2 = boost::target(*ei, g);
			adjacentEdges.push_back(*ei);
		}
		SortAdjacentVertices(g, *vi, adjacentEdges);
		for(int i = 0; i < adjacentEdges.size(); ++i)
		{
			std::cout << *vi << " -> " << adjacentEdges[i] << std::endl;
		}
		if(adjacentEdges.size()>0)
			std::cout << std::endl;
		embedding[*vi] = adjacentEdges;
	}
#endif

	std::cout << std::endl << "Vertices on the faces: " << std::endl;
	vertex_output_visitor v_vis;
	planar_face_traversal(g, &embedding[0], v_vis);

	std::cout << std::endl << "Edges on the faces: " << std::endl;
	edge_output_visitor e_vis;
	planar_face_traversal(g, &embedding[0], e_vis);

	RemoveTheOutsideFace();
}

void CPlanarGraph::SortAdjacentVertices(const Graph& g, VertexDescriptor vert, std::vector<EdgeDescriptor>& adjacentEdges)
{
	if(adjacentEdges.size() < 1)
	{
		return;
	}

	VertexDescriptor v1_0 = boost::source(adjacentEdges[0], g);
	VertexDescriptor v2_0 = boost::target(adjacentEdges[0], g);
	VertexDescriptor v_0 = (v1_0 == vert) ? v2_0 : v1_0;
	v2f edgeRef = GetNode(v_0).GetPos() - GetNode(vert).GetPos();
	for ( int i = 0; i < adjacentEdges.size(); ++i )
	{
		for ( int j = i+1; j < adjacentEdges.size(); ++j )
		{
			VertexDescriptor v1_i = boost::source(adjacentEdges[i], g);
			VertexDescriptor v2_i = boost::target(adjacentEdges[i], g);
			VertexDescriptor v_i = (v1_i == vert) ? v2_i : v1_i;
			VertexDescriptor v1_j = boost::source(adjacentEdges[j], g);
			VertexDescriptor v2_j = boost::target(adjacentEdges[j], g);
			VertexDescriptor v_j = (v1_j == vert) ? v2_j : v1_j;
			v2f edgePr1 = GetNode(v_i).GetPos() - GetNode(vert).GetPos();
			v2f edgePr2 = GetNode(v_j).GetPos() - GetNode(vert).GetPos();
			if( CompareEdgeDirections(edgePr1, edgePr2, edgeRef) == true )
			{
				std::swap(adjacentEdges[i], adjacentEdges[j]);
			}
		}
	}
}

bool CPlanarGraph::CompareEdgeDirections(const v2f& edgePr1, const v2f& edgePr2, const v2f& edgeRef)
{
#if 0 // Old and wrong version without the reference
	v3f pr1 = v3f(edgePr1[0], edgePr1[1], 0.f);
	v3f pr2 = v3f(edgePr2[0], edgePr2[1], 0.f);
	v3f crossProduct = cross(pr1, pr2);
	bool flag =(crossProduct[2] < 0.f) ? true : false;
	return flag;
#else
	v3f pr1 = v3f(edgePr1[0], edgePr1[1], 0.f);
	v3f pr2 = v3f(edgePr2[0], edgePr2[1], 0.f);
	v3f pr0 = v3f(edgeRef[0], edgeRef[1], 0.f);
	v3f cp1 = cross(pr0, pr1);
	v3f cp2 = cross(pr0, pr2);
	// nothing is less than zero degrees
	if ( cp2[2] == 0.f && pr2[0] * pr0[0] + pr2[1] * pr0[1] >= 0.f )
	{
		return false;
	}
	// zero degrees is less than anything else
	if ( cp1[2] == 0.f && pr1[0] * pr0[0] + pr1[1] * pr0[1] >= 0.f )
	{
		return true;
	}
	if ( cp1[2] * cp2[2] >= 0.f )
	{
		// both on same side of reference, compare to each other
		v3f cp = cross(pr1, pr2);
		return cp[2] > 0.f;
	}
	// vectors "less than" zero degrees are actually large, near 2 pi
	return cp1[2] > 0.f;
#endif
}

void CPlanarGraph::RemoveTheOutsideFace()
{
	if ( m_faces.size() < 1 )
	{
		return;
	}
	int maxFaceIdx = -1;
	int maxFaceSize = -1;
	for ( int i=0; i<int(m_faces.size()); i++ )
	{
		int faceSizeTmp = m_faces[i].GetFaceSize();
		if ( faceSizeTmp > maxFaceSize )
		{
			maxFaceSize = faceSizeTmp;
			maxFaceIdx = i;
		}
	}
	std::cout << "To remove the " << maxFaceIdx << "th face of size " << m_faces[maxFaceIdx].GetFaceSize() << "...\n";
	std::vector<CGraphFace> facesNew;
	for ( int i=0; i<int(m_faces.size()); i++ )
	{
		if ( i == maxFaceIdx )
		{
			continue;
		}
		facesNew.push_back(m_faces[i]);
	}
	m_faces = facesNew;

	for ( int i=0; i<int(m_faces.size()); i++ )
	{
		std::cout << "The " << i << " th face: ";
		for ( int j=0; j<m_faces[i].GetFaceSize(); j++ )
		{
			std::cout << m_faces[i].GetIndices()[j] << " ";
		}
		std::cout << std::endl;
	}
}

bool CPlanarGraph::VisitedAllNodes()
{
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		if ( GetNode(i).GetFlagVisited() == false )
		{
			return false;
		}
	}
	return true;
}

bool CPlanarGraph::VisitedNoNode()
{
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		if ( GetNode(i).GetFlagVisited() == true )
		{
			return false;
		}
	}
	return true;
}

bool CPlanarGraph::HasFixedNode()
{
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		if ( GetNode(i).GetFlagFixed() == true )
		{
			return true;
		}
	}
	return false;
}

std::vector<int> CPlanarGraph::GetFixedNodes()
{
	std::vector<int> indices;
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		if ( GetNode(i).GetFlagFixed() == true )
		{
			indices.push_back(i);
		}
	}
	return indices;
}

std::vector<int> CPlanarGraph::GetUnfixedNodes()
{
	std::vector<int> indices;
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		if ( GetNode(i).GetFlagFixed() == false )
		{
			indices.push_back(i);
		}
	}
	return indices;
}

std::vector<int> CPlanarGraph::ExtractDeepestFaceOrChain(bool& flagCyclic, bool flagSmallFaceFirst)
{
	if ( VisitedNoNode() == true && HasFixedNode() == true )
	{
		// Handle the fixed nodes first...
		std::vector<int> indices = GetFixedNodes();
		return indices;
	}
	if ( m_chains.empty() == false )
	{
		for ( int i=0; i<int(m_chains.size()); i++ )
		{
			CGraphChain& chain = GetChain(i);
#if 0 // Before 09/23/2013, which is NOT suitable for ghost cases
			int idx0 = chain.GetIndices()[0];
			if ( GetNode(idx0).GetFlagVisited() == true )
			{
				continue;
			}
#else
			bool flagAllVisited = true;
			for ( int j=0; j<chain.GetChainSize(); j++ )
			{
				if ( GetNode(chain.GetIndices()[j]).GetFlagVisited() == false )
				{
					flagAllVisited = false;
					break;
				}
			}
			if ( flagAllVisited == true )
			{
				continue;
			}
#endif
			flagCyclic = chain.GetFlagCyclic();
			std::vector<int> indices = chain.GetIndices();
			return indices;
		}
	}
	std::vector<int> faceIndices = ExtractDeepestFace(flagSmallFaceFirst);
	if ( faceIndices.empty() == false && VisitedNoNode() == true )
	{
		flagCyclic = true;
		return faceIndices;
	}
	int faceConstraintCount = CountConstraints(faceIndices);
	std::vector<int> chainIndices = ExtractDeepestChainNew();
	int chainConstraintCount = CountConstraints(chainIndices);
	if ( faceIndices.empty() == false && faceConstraintCount > chainConstraintCount )
	{
		flagCyclic = true;
		return faceIndices;
	}
	else
	{
		flagCyclic = false;
		return chainIndices;
	}
}

std::vector<int> CPlanarGraph::ExtractDeepestFaceOrChainOld(bool& flagCyclic, bool flagSmallFaceFirst)
{
	std::vector<int> faceIndices = ExtractDeepestFace(flagSmallFaceFirst);
	if ( faceIndices.empty() == false )
	{
		flagCyclic = true;
		return faceIndices;
	}
	flagCyclic = false;
	std::vector<int> chainIndices = ExtractDeepestChain();
	return chainIndices;
}

std::vector<int> CPlanarGraph::ExtractDeepestFace(bool flagSmallFaceFirst)
{
	std::vector<int> faceIndices;
	int deepestFaceIdx = -1;
	int deepestFaceSize = 0;
	int smallestFaceIdx = -1;
	int smallestFaceSize = std::numeric_limits<int>::max();
	for ( int i=0; i<GetNumOfFaces(); i++ )
	{
		CGraphFace& face = GetFace(i);
		std::vector<int>& indices = face.GetIndices();
		bool flagAllNodesUnvisited = true;
		for ( int j=0; j<face.GetFaceSize(); j++ )
		{
			if ( GetNode(indices[j]).GetFlagVisited() == true )
			{
				flagAllNodesUnvisited = false;
				break;
			}
		}
		if ( flagAllNodesUnvisited == false )
		{
			continue;
		}
		if ( face.GetFaceSize() > deepestFaceSize )
		{
			deepestFaceIdx = i;
			deepestFaceSize = face.GetFaceSize();
		}
		if ( face.GetFaceSize() < smallestFaceSize )
		{
			smallestFaceIdx = i;
			smallestFaceSize = face.GetFaceSize();
		}
	}
	if ( deepestFaceIdx >= 0 )
	{
		faceIndices = GetFace(deepestFaceIdx).GetIndices();
	}
	if ( flagSmallFaceFirst == true && smallestFaceIdx >= 0 )
	{
		faceIndices = GetFace(smallestFaceIdx).GetIndices();
	}
	return faceIndices;
}

std::vector<int> CPlanarGraph::ExtractDeepestChainNew()
{
	int chainLengthMin = std::numeric_limits<int>::max();
	int chainLengthMax = 0;
	int chainLengthMinC = 0;
	int chainLengthMaxC = 0;
	std::vector<int> chainMin;
	std::vector<int> chainMax;
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		if ( GetNode(i).GetFlagVisited() == true )
		{
			continue;
		}
		std::vector<int> chainIndices;
		int idx = i;
		chainIndices.push_back(idx);
		GetNode(idx).SetFlagVisited(true);
		// Grow the chain with unvisited nodes...
		bool flagInserted = true;
		while ( flagInserted )
		{
			flagInserted = false;
			std::vector<int>& neighbors = GetNode(idx).GetNeighbors();
			for ( int j=0; j<int(neighbors.size()); j++ )
			{
				int idxTmp = neighbors[j];
				if ( GetNode(idxTmp).GetFlagVisited() == false )
				{
					idx = idxTmp;
					chainIndices.push_back(idx);
					GetNode(idx).SetFlagVisited(true);
					flagInserted = true;
					break;
				}
			}
		}
		int chainLengthTmp = int(chainIndices.size());
		// Set the visited flags back to false...
		for ( int j=0; j<chainLengthTmp; j++ )
		{
			GetNode(chainIndices[j]).SetFlagVisited(false);
		}
		int chainConstraintCount = CountConstraints(chainIndices);
		if ( chainConstraintCount>= chainLengthMinC && chainLengthTmp < chainLengthMin )
		{
			chainLengthMinC = chainConstraintCount;
			chainLengthMin = chainLengthTmp;
			chainMin = chainIndices;
		}
		if ( chainConstraintCount>= chainLengthMaxC && chainLengthTmp > chainLengthMax )
		{
			chainLengthMaxC = chainConstraintCount;
			chainLengthMax = chainLengthTmp;
			chainMax = chainIndices;
		}
	}
	return chainMin;
}

std::vector<int> CPlanarGraph::ExtractDeepestChain()
{
	std::vector<int> chainIndices;
	// Almost randomly pick an unvisited node as the start of the chain...
	int idx = -1;
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		if ( GetNode(i).GetFlagVisited() == false )
		{
			idx = i;
			chainIndices.push_back(idx);
			GetNode(idx).SetFlagVisited(true);
			break;
		}
	}
	// Grow the chain with unvisited nodes...
	bool flagInserted = true;
	while ( flagInserted )
	{
		flagInserted = false;
		std::vector<int>& neighbors = GetNode(idx).GetNeighbors();
		for ( int i=0; i<int(neighbors.size()); i++ )
		{
			int idxTmp = neighbors[i];
			if ( GetNode(idxTmp).GetFlagVisited() == false )
			{
				idx = idxTmp;
				chainIndices.push_back(idx);
				GetNode(idx).SetFlagVisited(true);
				flagInserted = true;
				break;
			}
		}
	}
	// Set the visited flags back to false...
	for ( int i=0; i<int(chainIndices.size()); i++ )
	{
		GetNode(chainIndices[i]).SetFlagVisited(false);
	}
	return chainIndices;
}

int CPlanarGraph::CountConstraints(std::vector<int>& indices)
{
	int count = 0;
	for ( int i=0; i<int(indices.size()); i++ )
	{
		int idx = indices[i];
		std::vector<int>& neighbors = GetNode(idx).GetNeighbors();
		for ( int j=0; j<int(neighbors.size()); j++ )
		{
			int idxTmp = neighbors[j];
			if ( GetNode(idxTmp).GetFlagVisited() == true )
			{
				count ++;
				break;
			}
		}
	}
	return count;
}

void CPlanarGraph::GetGraphBoundingBox(v2f& posMin, v2f& posMax)
{
	v2f pMin(1e10);
	v2f pMax(-1e10);
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		v2f pi = GetNode(i).GetPos();
		for ( int j=0; j<2; j++ )
		{
			pMin[j] = min(pMin[j], pi[j]);
			pMax[j] = max(pMax[j], pi[j]);
		}
	}
	posMin = pMin;
	posMax = pMax;
}

void CPlanarGraph::MoveGraphToSceneCenter()
{
	v2f posMin, posMax;
	GetGraphBoundingBox(posMin, posMax);
	v2f posCen = (posMin + posMax) * 0.5f;
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		v2f pi = GetNode(i).GetPos();
		pi = pi - posCen;
		GetNode(i).SetPos(pi);
	}
}

void CPlanarGraph::ScaleGraphNodePositions(float scaling)
{
	if ( scaling <= 0.f )
	{
		return;
	}
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		v2f pi = GetNode(i).GetPos();
		pi = pi * scaling;
		GetNode(i).SetPos(pi);
	}
}

bool CPlanarGraph::LoadChainsFromTXT(const char* fileName)
{
	m_chains.clear();
	std::ifstream fin(fileName);
	if ( fin.fail() == true )
	{
		std::cout << "Failed to load graph chains from file " << fileName << "!\n";
		return false;
	}

	int numOfChains;
	fin >> numOfChains;
	for ( int i=0; i<numOfChains; i++ )
	{
		CGraphChain chain;
		bool flag;
		fin >> flag;
		chain.SetFlagCyclic(flag);
		int numOfIndices;
		fin >> numOfIndices;
		for ( int j=0; j<numOfIndices; j++ )
		{
			int idx;
			fin >> idx;
			chain.AddIndex(idx);
		}
		m_chains.push_back(chain);
	}

	return true;
}

int CPlanarGraph::FindNodeAccordingToName(const char* str)
{
	for ( int i=0; i<GetNumOfNodes(); i++ )
	{
		if ( GetNode(i).GetName() == std::string(str) )
		{
			return i;
		}
	}
	std::cout << "Cannot find a node named " << str << "!\n";
	return -1;
}

void CPlanarGraph::RemoveIndividualNodes()
{
	int numOfNodes = GetNumOfNodes();
	std::vector<bool> vecKeepFlags(numOfNodes, false);
	std::vector<int> vecNewIndices(numOfNodes, -1);
	std::vector<CGraphNode> nodesToKeep;
	int keepCount = 0;
	for ( int i=0; i<numOfNodes; i++ )
	{
		CGraphNode& node = GetNode(i);
		if ( node.GetNeighbors().empty() == false )
		{
			nodesToKeep.push_back(node);
			vecKeepFlags[i] = true;
			vecNewIndices[i] = keepCount;
			keepCount ++;
		}
	}
	if ( keepCount == numOfNodes )
	{
		return;
	}
	m_nodes = nodesToKeep;
	for ( int i=0; i<GetNumOfEdges(); i++ )
	{
		CGraphEdge& edge = GetEdge(i);
		int idx0 = edge.GetIdx0();
		int idx1 = edge.GetIdx1();
		idx0 = vecNewIndices[idx0];
		idx1 = vecNewIndices[idx1];
		edge.SetIdx0(idx0);
		edge.SetIdx1(idx1);
	}

	SetNodeNeighbors();
}
