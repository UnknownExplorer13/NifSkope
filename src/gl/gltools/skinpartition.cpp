/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#include "skinpartition.h"
#include "model/nifmodel.h"

SkinPartition::SkinPartition( const NifModel * nif, const QModelIndex & index )
{
	numWeightsPerVertex = nif->get<int>( index, "Num Weights Per Vertex" );

	vertexMap = nif->getArray<int>( index, "Vertex Map" );

	if ( vertexMap.isEmpty() ) {
		vertexMap.resize( nif->get<int>( index, "Num Vertices" ) );

		for ( int x = 0; x < vertexMap.count(); x++ )
			vertexMap[ x ] = x;
	}

	boneMap = nif->getArray<int>( index, "Bones" );

	QModelIndex iWeights = nif->getIndex( index, "Vertex Weights" );
	QModelIndex iBoneIndices = nif->getIndex( index, "Bone Indices" );

	weights.resize( vertexMap.count() * numWeightsPerVertex );

	for ( int v = 0; v < vertexMap.count(); v++ ) {
		for ( int w = 0; w < numWeightsPerVertex; w++ ) {
			QModelIndex iw = iWeights.child( v, 0 ).child( w, 0 );
			QModelIndex ib = iBoneIndices.child( v, 0 ).child( w, 0 );

			weights[ v * numWeightsPerVertex + w ].first = ( ib.isValid() ? nif->get<int>( ib ) : 0 );
			weights[ v * numWeightsPerVertex + w ].second = ( iw.isValid() ? nif->get<float>( iw ) : 0 );
		}
	}

	QModelIndex iStrips = nif->getIndex( index, "Strips" );

	for ( int s = 0; s < nif->rowCount( iStrips ); s++ ) {
		tristrips << nif->getArray<quint16>( iStrips.child( s, 0 ) );
	}

	triangles = nif->getArray<Triangle>( index, "Triangles" );
}

QVector<Triangle> SkinPartition::getRemappedTriangles() const
{
	QVector<Triangle> tris;

	for ( const auto & t : triangles )
		tris << Triangle( vertexMap[ t.v1() ], vertexMap[ t.v2() ], vertexMap[ t.v3() ] );

	return tris;
}

QVector<QVector<quint16>> SkinPartition::getRemappedTristrips() const
{
	QVector<QVector<quint16>> tris;

	for ( const auto & t : tristrips ) {
		QVector<quint16> points;
		for ( const auto & p : t )
			points << vertexMap[ p ];
		tris << points;
	}

	return tris;
}
