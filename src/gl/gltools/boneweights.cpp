#include "boneweights.h"
#include "boundsphere.h"
#include "model/nifmodel.h"

BoneWeights::BoneWeights( const NifModel * nif, const QModelIndex & index, int b, int vcnt )
{
	trans = Transform( nif, index );
	auto sph = BoundSphere( nif, index );
	center = sph.center;
	radius = sph.radius;
	bone = b;

	QModelIndex idxWeights = nif->getIndex( index, "Vertex Weights" );
	if ( vcnt && idxWeights.isValid() ) {
		for ( int c = 0; c < nif->rowCount( idxWeights ); c++ ) {
			QModelIndex idx = idxWeights.child( c, 0 );
			weights.append( VertexWeight( nif->get<int>( idx, "Index" ), nif->get<float>( idx, "Weight" ) ) );
		}
	}
}

void BoneWeights::setTransform( const NifModel * nif, const QModelIndex & index )
{
	trans = Transform( nif, index );
	auto sph = BoundSphere( nif, index );
	center = sph.center;
	radius = sph.radius;
}

BoneWeightsUNorm::BoneWeightsUNorm(QVector<quint32> unorms, int v)
{
	weights.resize(unorms.size());
	for ( int i = 0; i < unorms.size(); i++ ) {
		weights[i] = VertexWeight(v, unorms[i] / (float)UINT_MAX);
	}
}
