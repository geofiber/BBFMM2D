//	This Source Code Form is subject to the terms of the Mozilla Public
//	License, v. 2.0. If a copy of the MPL was not distributed with this
//	file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
//	<author>Sivaram Ambikasaran, Ruoxi Wang</author>
//	
//	H2_2D_tree.cpp
//

#include"H2_2D_tree.hpp"
H2_2D_tree::H2_2D_tree(const unsigned short nchebnodes, const MatrixXd& charge, const VectorXd* location){
	this->nchebnodes	=	nchebnodes;
	this->rank          =	nchebnodes*nchebnodes;
	this->N             =	charge.rows();
	this->m             =	charge.cols();
	this->maxlevels		=	0;

	this->charge_tree	=	charge;
	for(int k=0; k<2; ++k){
		this->location_tree[k]	=	location[k];
	}
    //	Get Chebyshev nodes
	cnode			=	VectorXd(nchebnodes);
	get_standard_Chebyshev_nodes(nchebnodes,cnode);
    //	Get Chebyshev polynomials evaluated at Chebyshev nodes
	Tnode			=	MatrixXd(nchebnodes,nchebnodes);
	get_standard_Chebyshev_polynomials(nchebnodes,nchebnodes,cnode,Tnode);
    //	Gets transfer matrices
	gettransfer(nchebnodes,cnode,Tnode,R);
	nlevels			=	0;
	get_center_radius(location, center, radius);
    //	Create root
	root			=	new H2_2D_node(0, 0);
	root->nneighbor		=	0;
	root->ninteraction	=	0;
	root->N			=	N;
	for(int k=0;k<2;++k){
		root->center[k]	=	center[k];
		root->radius[k]	=	radius[k];
	}
	root->index.setLinSpaced(N,0,N-1);
    std::cout << "Assigning children..." << std::endl;
	assignchildren(root);
	std::cout << "Assigned children." << std::endl;
    build_tree(root);
    std::cout << "Maximum levels is: " << this->maxlevels << std::endl;
}


void H2_2D_tree::deletetree(H2_2D_node*& node){
	if(node!=NULL){
		for(unsigned short k=0; k<4; ++k){
			deletetree(node->child[k]);
		}
		delete node;
		node = NULL;
	}
}

//	Assigns children;
void H2_2D_tree::assignchildren(H2_2D_node*& node){
	if(node->N==0){
		node->isleaf	=	true;
		node->isempty	=	true;
	}
	else{
		node->potential		=	MatrixXd::Zero(node->N,m);
		node->nodepotential	=	MatrixXd::Zero(rank,m);
		node->nodecharge	=	MatrixXd::Zero(rank,m);

		for(unsigned short j=0; j<2; ++j){
			node->location[j]		=	VectorXd(node->N);
			getscaledchebnode(nchebnodes, cnode, node->center[j], node->radius[j], node->scaledcnode[j]);
			for(unsigned long k=0;k<node->N;++k){
				node->location[j](k)=	location_tree[j](node->index(k));
			}
		}

		if(node->N<= (unsigned long) 4*rank){
			node->isleaf	=	true;
			get_transfer_from_parent_to_children(nchebnodes,node->location,node->center,node->radius,cnode,Tnode,node->R);
			getcharge(node);
			node->nodecharge=	node->nodecharge+node->R.transpose()*node->charge;
			if(this->maxlevels<node->nlevel){
				this->maxlevels	=	node->nlevel;
			}
		}
		else{
			for(unsigned short k=0; k<4; ++k){
				node->child[k]			=	new H2_2D_node(node->nlevel+1, k);
				node->child[k]->parent		=	node;
				node->child[k]->center[0]	=	node->center[0]+((k%2)-0.5)*node->radius[0];
				node->child[k]->center[1]	=	node->center[1]+((k/2)-0.5)*node->radius[1];
				for(unsigned short j=0; j<2; ++j){
					node->child[k]->radius[j]	=	0.5*node->radius[j];
				}
			node->child[k]->N		=	0;
			}
			for(unsigned long k=0; k<node->N; ++k){
				if(location_tree[0](node->index(k))<node->center[0]){
					if(location_tree[1](node->index(k))<node->center[1]){
						node->child[0]->index.conservativeResize(node->child[0]->N+1);
						node->child[0]->index(node->child[0]->N)	=	node->index(k);
						++node->child[0]->N;
					}
					else{
						node->child[2]->index.conservativeResize(node->child[2]->N+1);
						node->child[2]->index(node->child[2]->N)	=	node->index(k);
						++node->child[2]->N;
					}
				}
				else{
					if(location_tree[1](node->index(k))<node->center[1]){
						node->child[1]->index.conservativeResize(node->child[1]->N+1);
						node->child[1]->index(node->child[1]->N)	=	node->index(k);
						++node->child[1]->N;
					}
					else{
						node->child[3]->index.conservativeResize(node->child[3]->N+1);
						node->child[3]->index(node->child[3]->N)	=	node->index(k);
						++node->child[3]->N;
					}
				}
			}

			for(unsigned short k=0; k<4; ++k){
				assignchildren(node->child[k]);
				node->nodecharge=	node->nodecharge+R[k].transpose()*(node->child[k]->nodecharge);
			}
		}
	}
}

//	Calculates potential;
void H2_2D_tree::build_tree(H2_2D_node*& node){
	if(!node->isempty){
		if(!node->isleaf){
			assignsiblings(node);
			for(unsigned short k=0;k<8;++k){
				if(node->neighbor[k]!=NULL){
					if(!node->neighbor[k]->isleaf){
                        assigncousin(node,k);
					}
				}
			}
            for(unsigned short k=0;k<4;++k) {
                build_tree(node->child[k]);
            }
        }
    }
}



//	Obtains charge to node when needed;
void H2_2D_tree::getcharge(H2_2D_node*& node){
	if(node->charge_computed==true){
		return;
	}
	else{
		node->charge_computed	=	true;
		node->charge		=	MatrixXd::Zero(node->N,m);
		for(unsigned long k=0;k<node->N;++k){
			node->charge.row(k)	=	charge_tree.row(node->index(k));
		}
	}
}


//	Obtains standard Chebyshev nodes in interval [-1,1];
void H2_2D_tree::get_standard_Chebyshev_nodes(const unsigned short nchebnodes, VectorXd& cnode){
	cnode	=	VectorXd(nchebnodes);
	for(unsigned short k=0;k<nchebnodes;++k){
		cnode(k)	=	-cos((k+0.5)*PI/nchebnodes);
	}
}

//	Obtains standard Chebyshev polynomials evaluated at given set of points;
void H2_2D_tree::get_standard_Chebyshev_polynomials(const unsigned short nchebpoly, const unsigned long N, const VectorXd& x, MatrixXd& T){
	T	=	MatrixXd(N,nchebpoly);
	T.col(0)=	VectorXd::Ones(N);
	if(nchebpoly>1){
		T.col(1)	=	x;
		for(unsigned short k=2; k<nchebpoly; ++k){
			T.col(k)=	2.0*x.cwiseProduct(T.col(k-1))-T.col(k-2);
		}
	}
}

//	Obtains center and radius of node location;
void H2_2D_tree::get_center_radius(const VectorXd* location, double* center, double* radius){
	double max_0	=	location[0].maxCoeff();
	double max_1	=	location[1].maxCoeff();
	double min_0	=	location[0].minCoeff();
	double min_1	=	location[1].minCoeff();
	center[0]	=	0.5*(max_0+min_0);
	center[1]	=	0.5*(max_1+min_1);
	radius[0]	=	0.5*(max_0-min_0);
	radius[1]	=	0.5*(max_1-min_1);
}

//	Obtains interpolation operator, which interpolates information from Chebyshev nodes of parent to points in children;
void H2_2D_tree::get_transfer_from_parent_to_children(const unsigned short nchebnodes, const VectorXd* location, const double* center, const double* radius, const VectorXd& cnode, const MatrixXd& Tnode, MatrixXd& R){
	unsigned long N		=	location[0].size();
	VectorXd standlocation[2];
	standlocation[0]	=	(location[0]-center[0]*VectorXd::Ones(N))/radius[0];
	standlocation[1]	=	(location[1]-center[1]*VectorXd::Ones(N))/radius[1];
	MatrixXd Transfer[2];
	for(unsigned short k=0;k<2;++k){
		get_standard_Chebyshev_polynomials(nchebnodes, N, standlocation[k], Transfer[k]);
		Transfer[k]	=	(2.0*Transfer[k]*Tnode.transpose()-MatrixXd::Ones(N,nchebnodes))/nchebnodes;
	}
	unsigned short rank	=	nchebnodes*nchebnodes;
	R			=	MatrixXd(N,rank);
	for(unsigned short k=0;k<N;++k){
		for(unsigned short i=0; i<nchebnodes; ++i){
			for(unsigned short j=0;j<nchebnodes;++j){
				R(k,i+nchebnodes*j)=Transfer[0](k,i)*Transfer[1](k,j);
			}
		}
	}
}

//	Obtains interpolation operator, which interpolates information from Chebyshev nodes of parent to Chebyshev nodes of children;
void H2_2D_tree::get_transfer_from_parent_cnode_to_children_cnode(const unsigned short nchebnodes, const VectorXd& cnode, const MatrixXd& Tnode, MatrixXd& Transfer){
	VectorXd childcnode(2*nchebnodes);
	childcnode.segment(0,nchebnodes)		=	0.5*(cnode-VectorXd::Ones(nchebnodes));
	childcnode.segment(nchebnodes,nchebnodes)	=	0.5*(cnode+VectorXd::Ones(nchebnodes));
	get_standard_Chebyshev_polynomials(nchebnodes, 2*nchebnodes, childcnode, Transfer);
	Transfer	=	(2.0*Transfer*Tnode.transpose()-MatrixXd::Ones(2*nchebnodes,nchebnodes))/nchebnodes;
}

//	Evaluates transfer from four children to parent;
void H2_2D_tree::gettransfer(const unsigned short nchebnodes, const VectorXd& cnode, const MatrixXd& Tnode, MatrixXd* R){
	MatrixXd S(2*nchebnodes,nchebnodes);
	get_transfer_from_parent_cnode_to_children_cnode(nchebnodes, cnode, Tnode, S);
	MatrixXd Transfer[2];
	Transfer[0]	=	S.block(0,0,nchebnodes,nchebnodes);
	Transfer[1]	=	S.block(nchebnodes,0,nchebnodes,nchebnodes);
	unsigned short rank	=	nchebnodes*nchebnodes;
	for(unsigned short k=0; k<4; ++k){
		R[k]	=	MatrixXd(rank,rank);
	}
	for(unsigned short i=0;i<nchebnodes;++i){
		for(unsigned short j=0;j<nchebnodes;++j){
			for(unsigned short k=0;k<nchebnodes;++k){
				for(unsigned short l=0;l<nchebnodes;++l){
					R[0](i*nchebnodes+j,k*nchebnodes+l)	=	Transfer[0](i,k)*Transfer[0](j,l);
					R[1](i*nchebnodes+j,k*nchebnodes+l)	=	Transfer[0](i,k)*Transfer[1](j,l);
					R[2](i*nchebnodes+j,k*nchebnodes+l)	=	Transfer[1](i,k)*Transfer[0](j,l);
					R[3](i*nchebnodes+j,k*nchebnodes+l)	=	Transfer[1](i,k)*Transfer[1](j,l);
				}
			}
		}
	}
}

//	Evaluates 'nchebnodes' standardized chebyshev nodes in any interval;
void H2_2D_tree::getscaledchebnode(const unsigned short& nchebnodes, const VectorXd& cnode, const double& center, const double& radius, VectorXd& chebnode){
	chebnode		=	VectorXd(nchebnodes);
	for(unsigned short k=0;k<nchebnodes;++k){
		chebnode(k)	=	center+radius*cnode(k);
	}
}


//	Displays desired information;
void H2_2D_tree:: display(H2_2D_node* node){
	std::cout << node->nlevel << ": " << node->N << std::endl << node->index << std::endl;
	if(!node->isleaf){
		for(unsigned short k=0; k<4; ++k){
			display(node->child[k]);
		}
	}
}

//	Assign siblings to children of a the node
void H2_2D_tree:: assignsiblings(H2_2D_node*& node){
//	Assign siblings to child[0]
	node->child[0]->neighbor[3]	=	node->child[1];
	node->child[0]->neighbor[5]	=	node->child[2];
	node->child[0]->neighbor[4]	=	node->child[3];

//	Assign siblings to child[1]
	node->child[1]->neighbor[7]	=	node->child[0];
	node->child[1]->neighbor[6]	=	node->child[2];
	node->child[1]->neighbor[5]	=	node->child[3];

//	Assign siblings to child[2]
	node->child[2]->neighbor[1]	=	node->child[0];
	node->child[2]->neighbor[2]	=	node->child[1];
	node->child[2]->neighbor[3]	=	node->child[3];

//	Assign siblings to child[3]
	node->child[3]->neighbor[0]	=	node->child[0];
	node->child[3]->neighbor[1]	=	node->child[1];
	node->child[3]->neighbor[7]	=	node->child[2];

	for(unsigned short k=0;k<4;++k){
		node->child[k]->nneighbor+=3;
	}
}

//	Assign cousins to children of the node
void H2_2D_tree:: assigncousin(H2_2D_node*& node, unsigned short neighbor_number){
//	Assigning children of neighbor 0
	if(neighbor_number==0){
//	Assigning the cousins to child0. One neighbor and three well-separated cousins.
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[0]->child[0];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[0]->child[1];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[0]->child[2];
		node->child[0]->neighbor[0]					=	node->neighbor[0]->child[3];

//	Assigning the cousins to child1. Four well-separated cousins.
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[0]->child[0];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[0]->child[1];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[0]->child[2];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[0]->child[3];

//	Assigning the cousins to child2. Four well-separated cousins.
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[0]->child[0];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[0]->child[1];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[0]->child[2];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[0]->child[3];

//	Assigning the cousins to child3. Four well-separated cousins.
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[0]->child[0];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[0]->child[1];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[0]->child[2];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[0]->child[3];

//	Update neighbor count.

		node->child[0]->nneighbor					=	node->child[0]->nneighbor+1;
	}
//	Assigning children of neighbor 1
	else if(neighbor_number==1){
//	Assigning the cousins to child0. One neighbor and three well-separated cousins.
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[1]->child[0];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[1]->child[1];
		node->child[0]->neighbor[1]					=	node->neighbor[1]->child[2];
		node->child[0]->neighbor[2]					=	node->neighbor[1]->child[3];

//	Assigning the cousins to child1. Four well-separated cousins.

		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[1]->child[0];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[1]->child[1];
		node->child[1]->neighbor[0]					=	node->neighbor[1]->child[2];
		node->child[1]->neighbor[1]					=	node->neighbor[1]->child[3];

//	Assigning the cousins to child2. Four well-separated cousins.

		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[1]->child[0];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[1]->child[1];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[1]->child[2];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[1]->child[3];

//	Assigning the cousins to child3. Four well-separated cousins.

		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[1]->child[0];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[1]->child[1];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[1]->child[2];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[1]->child[3];

//	Update neighbor count.

		node->child[0]->nneighbor					=	node->child[0]->nneighbor+2;
		node->child[1]->nneighbor					=	node->child[1]->nneighbor+2;
	}
//	Assigning children of neighbor 2
	else if (neighbor_number==2){
//	Assigning the cousins to child0. One neighbor and three well-separated cousins.

		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[2]->child[0];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[2]->child[1];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[2]->child[2];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[2]->child[3];

//	Assigning the cousins to child1. Four well-separated cousins.

		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[2]->child[0];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[2]->child[1];
		node->child[1]->neighbor[2]					=	node->neighbor[2]->child[2];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[2]->child[3];

//	Assigning the cousins to child2. Four well-separated cousins.

		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[2]->child[0];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[2]->child[1];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[2]->child[2];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[2]->child[3];

//	Assigning the cousins to child3. Four well-separated cousins.

		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[2]->child[0];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[2]->child[1];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[2]->child[2];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[2]->child[3];

//	Update neighbor count.

		node->child[1]->nneighbor					=	node->child[1]->nneighbor+1;
	}
//	Assigning children of neighbor 3
	else if(neighbor_number==3){
//	Assigning the cousins to child0. One neighbor and three well-separated cousins.

		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[3]->child[0];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[3]->child[1];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[3]->child[2];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[3]->child[3];

//	Assigning the cousins to child1. Four well-separated cousins.

		node->child[1]->neighbor[3]					=	node->neighbor[3]->child[0];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[3]->child[1];
		node->child[1]->neighbor[4]					=	node->neighbor[3]->child[2];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[3]->child[3];

//	Assigning the cousins to child2. Four well-separated cousins.

		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[3]->child[0];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[3]->child[1];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[3]->child[2];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[3]->child[3];

//	Assigning the cousins to child3. Four well-separated cousins.

		node->child[3]->neighbor[2]					=	node->neighbor[3]->child[0];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[3]->child[1];
		node->child[3]->neighbor[3]					=	node->neighbor[3]->child[2];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[3]->child[3];

//	Update neighbor count.

		node->child[1]->nneighbor					=	node->child[1]->nneighbor+2;
		node->child[3]->nneighbor					=	node->child[3]->nneighbor+2;
	}
//	Assigning children of neighbor 4
	else if(neighbor_number==4){
//	Assigning the cousins to child0. One neighbor and three well-separated cousins.

		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[4]->child[0];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[4]->child[1];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[4]->child[2];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[4]->child[3];

//	Assigning the cousins to child1. Four well-separated cousins.

		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[4]->child[0];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[4]->child[1];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[4]->child[2];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[4]->child[3];

//	Assigning the cousins to child2. Four well-separated cousins.

		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[4]->child[0];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[4]->child[1];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[4]->child[2];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[4]->child[3];

//	Assigning the cousins to child3. Four well-separated cousins.

		node->child[3]->neighbor[4]					=	node->neighbor[4]->child[0];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[4]->child[1];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[4]->child[2];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[4]->child[3];

//	Update neighbor count.

		node->child[3]->nneighbor					=	node->child[3]->nneighbor+1;
	}
//	Assigning children of neighbor 5
	else if(neighbor_number==5){
//	Assigning the cousins to child0. One neighbor and three well-separated cousins.

		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[5]->child[0];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[5]->child[1];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[5]->child[2];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[5]->child[3];

//	Assigning the cousins to child1. Four well-separated cousins.

		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[5]->child[0];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[5]->child[1];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[5]->child[2];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[5]->child[3];

//	Assigning the cousins to child2. Four well-separated cousins.

		node->child[2]->neighbor[5]					=	node->neighbor[5]->child[0];
		node->child[2]->neighbor[4]					=	node->neighbor[5]->child[1];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[5]->child[2];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[5]->child[3];

//	Assigning the cousins to child3. Four well-separated cousins.

		node->child[3]->neighbor[6]					=	node->neighbor[5]->child[0];
		node->child[3]->neighbor[5]					=	node->neighbor[5]->child[1];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[5]->child[2];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[5]->child[3];

//	Update neighbor count.

		node->child[2]->nneighbor					=	node->child[2]->nneighbor+2;
		node->child[3]->nneighbor					=	node->child[3]->nneighbor+2;
}
//	Assigning children of neighbor 6
	else if (neighbor_number==6){
//	Assigning the cousins to child0. One neighbor and three well-separated cousins.

		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[6]->child[0];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[6]->child[1];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[6]->child[2];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[6]->child[3];

//	Assigning the cousins to child1. Four well-separated cousins.

		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[6]->child[0];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[6]->child[1];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[6]->child[2];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[6]->child[3];

//	Assigning the cousins to child2. Four well-separated cousins.

		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[6]->child[0];
		node->child[2]->neighbor[6]					=	node->neighbor[6]->child[1];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[6]->child[2];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[6]->child[3];

//	Assigning the cousins to child3. Four well-separated cousins.

		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[6]->child[0];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[6]->child[1];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[6]->child[2];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[6]->child[3];

//	Update neighbor count.

		node->child[2]->nneighbor					=	node->child[2]->nneighbor+1;
	}
//	Assigning children of neighbor 7
	else if (neighbor_number==7){
//	Assigning the cousins to child0. One neighbor and three well-separated cousins.

		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[7]->child[0];
		node->child[0]->neighbor[7]					=	node->neighbor[7]->child[1];
		node->child[0]->interaction[node->child[0]->ninteraction++]	=	node->neighbor[7]->child[2];
		node->child[0]->neighbor[6]					=	node->neighbor[7]->child[3];

//	Assigning the cousins to child1. Four well-separated cousins.

		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[7]->child[0];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[7]->child[1];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[7]->child[2];
		node->child[1]->interaction[node->child[1]->ninteraction++]	=	node->neighbor[7]->child[3];

//	Assigning the cousins to child2. Four well-separated cousins.

		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[7]->child[0];
		node->child[2]->neighbor[0]					=	node->neighbor[7]->child[1];
		node->child[2]->interaction[node->child[2]->ninteraction++]	=	node->neighbor[7]->child[2];
		node->child[2]->neighbor[7]					=	node->neighbor[7]->child[3];

//	Assigning the cousins to child3. Four well-separated cousins.

		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[7]->child[0];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[7]->child[1];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[7]->child[2];
		node->child[3]->interaction[node->child[3]->ninteraction++]	=	node->neighbor[7]->child[3];

//	Update neighbor count.

		node->child[0]->nneighbor					=	node->child[0]->nneighbor+2;
		node->child[2]->nneighbor					=	node->child[2]->nneighbor+2;
	}
}

H2_2D_tree::~H2_2D_tree() {
    deletetree(root);
}



