#pragma once

#include "TreeNode.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <climits>

class MonteCarloTree {
public:
	std::unique_ptr<TreeNode> root;
	std::vector<TreeNode*> path;
	board root_board;
	
	std::random_device rd;
	std::default_random_engine eng;

	static constexpr double c_parameter = sqrt(2.0);

	MonteCarloTree() : root(), path(), root_board(), eng(rd()) {}
	
	TreeNode* UCB (TreeNode* n)  {

		if(n->c_size == 0) return nullptr;
		
		constexpr double eps = 1e-3;
		double max_score = INT_MIN;
		std::size_t same_score[100]{};
		std::size_t idx = 0;

		for (std::size_t i = 0; i < n->c_size; ++i) {
			TreeNode* ch = n->child.get() + i;
			
			const double exploit { ch->win / (ch->count + 1.0) };
			const double explore { sqrt( log(n->count) / (double)(ch->count+1.0) ) };
			const double score { exploit + c_parameter * explore };
			
			if ( (score <= (max_score + eps) ) && (score >= (max_score - eps) ) ) {
				same_score[idx] = i;
				idx++;
			}
			else if (score > max_score) {
				max_score = score;
				same_score[0] = i;
				idx = 1;
			}
		}
		
		std::shuffle(std::begin(same_score), std::begin(same_score) + idx, eng);
		std::size_t best_idx = same_score[0];
		
		return (n->child.get() + best_idx); 
	}

	void select(board &b) {

		TreeNode* current { root.get() };
	
		path.clear();
		path.push_back(current);

		while (current->child != nullptr && current->c_size != 0) {
			current = UCB(current);
			path.push_back(current);

			b.move(current->move.prev, current->move.next, current->color);
		}
	}
	
	WIN_STATE simulate(board &b) {

		std::size_t count_step = 0;
		
		constexpr std::size_t limit_step = 100;
		while (true) {
			count_step++;
			// Game draw if exceed the limit step
			if (count_step > limit_step) {
				return b.compare_piece();
			}

			const PIECE& color { b.take_turn() };
			std::vector<Pair> ea { b.find_piece(color, EAT) };
			std::vector<Pair> mv { b.find_piece(color, MOVE) };
			
			if (!ea.empty()) {
				std::shuffle(ea.begin(), ea.end(), eng);
				b.move(ea[0].prev, ea[0].next, color);
			}
			else if (!mv.empty()){
				std::shuffle(mv.begin(), mv.end(), eng);
				b.move(mv[0].prev, mv[0].next, color);
			}
			else{
				if (color == BLACK)
					return WHITE_WIN;
				else
					return BLACK_WIN;
			}
		}
	}
	
	void backpropogate(const WIN_STATE &result) {
		for (auto &node : path) {
			node->addresult(result);
		}
	}
	
	void tree_policy() {
		board b {root_board};
		TreeNode *current;
		
		select(b);
		
		TreeNode &leaf_node = *(path.back());
		
		if (leaf_node.c_size==0 && leaf_node.count > 0){

			leaf_node.expand(b);

			if (leaf_node.c_size != 0) {
				current = UCB(&leaf_node);
				path.push_back(current);
				b.move(current->move.prev, current->move.next, current->color);
			}
			// no step can go
			else {
				const WIN_STATE result = ( (leaf_node.color==WHITE) ? WHITE_WIN : BLACK_WIN);
				backpropogate(result);
			}
		}

		const WIN_STATE result { simulate(b) };
		
		backpropogate(result);
	}
	

	void reset(board &b) {
		root_board = b;
		root = { std::make_unique<TreeNode>() };
		root->color = root_board.take_turn();
		root->move = {-1, -1};
		root->count = 1;
		root->win = 0;
		root->child = nullptr;
		root->c_size = 0;
		root->expand(b);
	}

};
