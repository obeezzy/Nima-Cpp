#ifndef _NIMA_ACTORIMAGE_HPP_
#define _NIMA_ACTORIMAGE_HPP_

#include "ActorNode.hpp"

namespace nima
{
	class Actor;
	class BlockReader;
	class ActorNode;

	enum BlendModes
	{
		Normal = 0,
		Multiply = 1,
		Screen = 2,
		Additive = 3
	};

	class ActorImage : public ActorNode
	{
		typedef ActorNode Base;
		private:
			int m_DrawOrder;
			BlendModes m_BlendMode;
			int m_TextureIndex;
			float* m_Vertices;
			unsigned short* m_Triangles;
			int m_VertexCount;
			int m_TriangleCount;
			float* m_AnimationDeformedVertices;
			bool m_IsVertexDeformDirty;
			float* m_BoneMatrices;

			struct BoneConnection
			{
				unsigned short boneIndex;
				ActorNode* node;
				Mat2D bind;
				Mat2D ibind;

				BoneConnection();
			};

			int m_NumConnectedBones;
			BoneConnection* m_BoneConnections;

		public:
			ActorImage();
			~ActorImage();
			ActorNode* makeInstance(Actor* resetActor);
			void copy(ActorImage* node, Actor* resetActor);
			void resolveNodeIndices(ActorNode** nodes);
			bool doesAnimationVertexDeform();
			void doesAnimationVertexDeform(bool doesIt);
			float* animationDeformedVertices();
			bool isVertexDeformDirty();
			void isVertexDeformDirty(bool isIt);
			int textureIndex();
			void disposeGeometry();

			int boneInfluenceMatricesLength();
			float* boneInfluenceMatrices();

			static ActorImage* read(Actor* actor, BlockReader* reader, ActorImage* node = NULL);
	};
}
#endif