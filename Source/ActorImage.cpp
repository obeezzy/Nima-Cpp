#include "ActorImage.hpp"
#include "ActorBone.hpp"
#include "BlockReader.hpp"
#include <cstring>

using namespace nima;

ActorImage::ActorImage() : 
		ActorNode(Node::Type::ActorImage),

		m_DrawOrder(0),
		m_BlendMode(BlendModes::Normal),
		m_TextureIndex(-1),
		m_Vertices(NULL),
		m_Triangles(NULL),
		m_VertexCount(0),
		m_TriangleCount(0),
		m_AnimationDeformedVertices(NULL),
		m_IsVertexDeformDirty(false),
		m_BoneMatrices(NULL),
		m_NumConnectedBones(0),
		m_BoneConnections(NULL)
{

}

ActorImage::BoneConnection::BoneConnection() : boneIndex(0), node(NULL)
{

}

ActorImage::~ActorImage()
{
	delete [] m_Vertices;
	delete [] m_Triangles;
	delete [] m_AnimationDeformedVertices;
	delete [] m_BoneMatrices;
}

ActorNode* ActorImage::makeInstance(Actor* resetActor)
{
	ActorImage* instanceNode = new ActorImage();
	instanceNode->copy(this, resetActor);
	return instanceNode;
}

bool ActorImage::doesAnimationVertexDeform()
{
	return m_AnimationDeformedVertices != NULL;
}

void ActorImage::doesAnimationVertexDeform(bool doesIt)
{
	if(doesIt)
	{
		m_AnimationDeformedVertices = new float [m_VertexCount * 2];
	}
	else
	{
		delete [] m_AnimationDeformedVertices;
		m_AnimationDeformedVertices = NULL;
	}
}

float* ActorImage::animationDeformedVertices()
{
	return m_AnimationDeformedVertices;	
}

bool ActorImage::isVertexDeformDirty()
{
	return m_IsVertexDeformDirty;
}

void ActorImage::isVertexDeformDirty(bool isIt)
{
	m_IsVertexDeformDirty = isIt;
}

void ActorImage::disposeGeometry()
{
	// Delete vertices only if we do not vertex deform at runtime.
	if(m_AnimationDeformedVertices == NULL)
	{
		delete [] m_Vertices;
		m_Vertices = NULL;
	}
	delete [] m_Triangles;
	m_Triangles = NULL;
}

int ActorImage::boneInfluenceMatricesLength()
{
	return m_NumConnectedBones == 0 ? 0 : (m_NumConnectedBones + 1) * 6;
}
float* ActorImage::boneInfluenceMatrices()
{
	if(m_BoneMatrices == NULL)
	{
		m_BoneMatrices = new float[boneInfluenceMatricesLength()];
		// First bone transform is always identity.
		m_BoneMatrices[0] = 1.0f;
		m_BoneMatrices[1] = 0.0f;
		m_BoneMatrices[2] = 0.0f;
		m_BoneMatrices[3] = 1.0f;
		m_BoneMatrices[4] = 0.0f;
		m_BoneMatrices[5] = 0.0f;
	}

	Mat2D mat;
	int bidx = 6;
	for(int i = 0; i < m_NumConnectedBones; i++)
	{
		BoneConnection& bc = m_BoneConnections[i];
		bc.node->updateTransforms();
		Mat2D::multiply(mat, bc.node->worldTransform(), bc.ibind);
		m_BoneMatrices[bidx++] = mat[0];
		m_BoneMatrices[bidx++] = mat[1];
		m_BoneMatrices[bidx++] = mat[2];
		m_BoneMatrices[bidx++] = mat[3];
		m_BoneMatrices[bidx++] = mat[4];
		m_BoneMatrices[bidx++] = mat[5];
	}

	return m_BoneMatrices;
}

void ActorImage::copy(ActorImage* node, Actor* resetActor)
{
	Base::copy(node, resetActor);

	m_DrawOrder = node->m_DrawOrder;
	m_BlendMode = node->m_BlendMode;
	m_TextureIndex = node->m_TextureIndex;
	m_VertexCount = node->m_VertexCount;
	m_TriangleCount = node->m_TriangleCount;
	m_Vertices = node->m_Vertices;
	m_Triangles = node->m_Triangles;
	if(node->m_AnimationDeformedVertices != NULL)
	{
		int deformedVertexLength = m_VertexCount * 2;
		m_AnimationDeformedVertices = new float[deformedVertexLength];
		std::memmove(m_AnimationDeformedVertices, node->m_AnimationDeformedVertices, deformedVertexLength * sizeof(float));
	}

	if(node->m_BoneConnections != NULL)
	{
		m_NumConnectedBones = node->m_NumConnectedBones;
		m_BoneConnections = new BoneConnection[node->m_NumConnectedBones];
		for(int i = 0; i < m_NumConnectedBones; i++)
		{
			BoneConnection& bcT = m_BoneConnections[i];
			BoneConnection& bcF = node->m_BoneConnections[i];

			bcT.boneIndex = bcF.boneIndex;
			Mat2D::copy(bcT.bind, bcF.bind);
			Mat2D::copy(bcT.ibind, bcF.ibind);
		} 
	}
}

ActorImage* ActorImage::read(Actor* actor, BlockReader* reader, ActorImage* node)
{
	if(node == NULL)
	{
		node = new ActorImage();
	}

	Base::read(actor, reader, node);

	bool isVisible = reader->readByte() != 0;
	if(isVisible)
	{
		node->m_BlendMode = (BlendModes)reader->readByte();
		node->m_DrawOrder = (int)reader->readUnsignedShort();
		node->m_TextureIndex = (int)reader->readByte();

		node->m_NumConnectedBones = (int)reader->readByte();
		if(node->m_NumConnectedBones != 0)
		{
			node->m_BoneConnections = new BoneConnection[node->m_NumConnectedBones];
			for(int i = 0; i < node->m_NumConnectedBones; i++)
			{
				BoneConnection& bc = node->m_BoneConnections[i];
				bc.boneIndex = reader->readUnsignedShort();
				reader->read(bc.bind);
				Mat2D::invert(bc.ibind, bc.bind);
			}

			Mat2D worldOverride;
			reader->read(worldOverride);
			node->overrideWorldTransform(worldOverride);
		}

		unsigned int numVertices = reader->readUnsignedInt();
		int vertexStride = node->m_NumConnectedBones > 0 ? 12 : 4;
		node->m_VertexCount = (int)numVertices;

		unsigned int vertexLength = numVertices * vertexStride;
		node->m_Vertices = new float[vertexLength];
		reader->readFloatArray(node->m_Vertices, vertexLength);

		unsigned int numTris = reader->readUnsignedInt();
		node->m_TriangleCount = (int)numTris;

		unsigned int triLength = numTris * 3;
		node->m_Triangles = new unsigned short[triLength];
		reader->readUnsignedShortArray(node->m_Triangles, triLength);
	}
	return node;
}

void ActorImage::resolveNodeIndices(ActorNode** nodes)
{
	Base::resolveNodeIndices(nodes);
	if(m_BoneConnections != NULL)
	{
		for(int i = 0; i < m_NumConnectedBones; i++)
		{
			BoneConnection& bc = m_BoneConnections[i];
			bc.node = nodes[bc.boneIndex];
			ActorBone* bone = reinterpret_cast<ActorBone*>(bc.node);
			bone->isConnectedToImage(true);
		}
	}
}

int ActorImage::textureIndex()
{
	return m_TextureIndex;
}