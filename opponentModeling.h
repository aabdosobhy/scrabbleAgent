#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>

#include "../simulation_testing/simulatoin_Test.h"
#include "constants.h"

using namespace std;


class opponentModeling {

	// all the combination from 0 to 8 
	// the equivalent of nCr is nCr[n][r] 
	int nCr[9][9];

	// the no of tiles to be constructed by all the combination of that size of tiles draws from the bag 
	// it acts as both
	//1- the remaining no. of tiles in the rack of the opponent 
	//2- the  no. of played tiles in the rack of the opponent that will be refilled 
	int rackSizeCombination;

	// the remaining unseen tiles in the game (all the tiles minus (the tiles on the board + the rack we have))
	// Must be sorted
	string bag;						

	// sorted non duplicate of all combination in the bag of a given size  
	vector<string> remainingTiles;

	// the tiles that the opponent has played 
	string tilesPlayed;

	// the frequency of the tiles in the bag 
	//tiles are represented from 0-25 as A-Z and the blank tile is represented as [=Z+1
	vector<int> charInBag;


	// just for testing 
	simulationTest simulate;

public:

	/*
	* constructor fills the combinations of nCr
	*/
	opponentModeling() {
		initialize();
	}


	/*
	* set the words in the bag and the frequency of each character in the bag
	*
	* @param bag	string of chars in he bag
	*
	*/
	void setBag(string bag) {

		sort(bag.begin(), bag.end());
		this->bag = bag;
		charInBag.clear();

		for (int i = 0; i < 27; i++)
			charInBag.push_back(0);

		for (int i = 0; i < bag.size(); i++)  // fill the bag 
			charInBag[bag[i] - firstIdx]++;

	}


	/*
	* estimate the rest of the oppononet rack and randomize the rest of the rack
	*
	* @param playedTiles	the played tiles the opponent had played
	*
	* @return the estimated with the refill from the bag
	*/

	vector<pair<string, int>> estimateOpponentsRack(string playedTiles) {

		tilesPlayed = playedTiles;
		rackSizeCombination = RACK_SIZE - tilesPlayed.size();
		vector<pair<string, int>> resultRacks;
		vector<pair<int, string>>estNonPlayed, resultRefillRack;
		priority_queue<pair<int, string>> rankedEstimatedTiles;

		int countReturnTiles = 0;
		// find all possible leaves 

		constructPossibleLeaves(bag, "", 0);
		// esitmate the Non Played Tiles and print it 
		estNonPlayed = esitmateNonPlayedTiles(tilesPlayed);


		// random from the remaining tiles 
		rackSizeCombination = RACK_SIZE - rackSizeCombination;
		for (int i = 0; i < estNonPlayed.size(); i++) {
			resultRefillRack = estimateRefillRack(estNonPlayed[i].second, bag, charInBag);
			for (int j = 0; j < resultRefillRack.size(); j++) {
				rankedEstimatedTiles.push({ resultRefillRack[j].first*estNonPlayed[i].first
					,estNonPlayed[i].second + resultRefillRack[j].second });
			}
		}


		while (!rankedEstimatedTiles.empty()) {
			if (countReturnTiles<maxReturnTiles) {
				resultRacks.push_back({ rankedEstimatedTiles.top().second,rankedEstimatedTiles.top().first });
				rankedEstimatedTiles.pop();
				countReturnTiles++;
			}
			else {
				break;
			}
		}


		// return -> concatenate the estimated with the random 

		return resultRacks;
	}


	/*
	* constructs a ranked (based on probability) possible tiles to draw from the bag
	*
	* @param bag	the tiles in the bag (all the possible tiles - ( tiles on board + remaining tiles of my rack + estimated opponent rack))
	*
	* @param refillTilesNo	the no. of tiles to be refilled
	*
	* @return all possible sub racks to refill from the bag
	*/

	vector<vector<char>> estimateRefillMyRack(string bag, int refillTilesNo) {
		priority_queue<pair<int, string>> rankedTiles;
		vector<vector<char>> ans;
		vector<int> wordsFreqInBag(27);
		int probLeave;
		for (int i = 0; i < bag.size(); i++)  // fill the bag 
			wordsFreqInBag[bag[i] - firstIdx]++;
		rackSizeCombination = refillTilesNo;

		remainingTiles.clear();

		constructPossibleLeaves(bag, "", 0);
		for (int i = 0; i < remainingTiles.size(); i++) {
			probLeave = drawFromBag(remainingTiles[i], wordsFreqInBag);
			rankedTiles.push({ probLeave,remainingTiles[i] });

		}

		while (!rankedTiles.empty()) {
			vector<char> temp;
			string leave = rankedTiles.top().second;
			for (int i = 0; i < refillTilesNo; i++)
				temp.push_back(leave[i]);
			ans.push_back(temp);
			rankedTiles.pop();
		}

		return ans;
	}

private:

	void initialize() {
		fillCombinations(8);

	}

	/*
	* ranks all the leaves by simulating the combinaion of the played tiles with the possible values
	* and rank them (if the result of the simulation is the same played tiles then that leave will be in our
	* possible racks else it will rank 0 ) by ranking the remaining on the probability of the expected leave
	*
	* @param rackPlayed		the tiles the opponent played on the board
	*
	* @return	ranked sub racks of the unseen tiles of the opponent rack (ranked pased on their probabilty )
	*/

	vector<pair<int, string>> esitmateNonPlayedTiles(string tilesPlayed) {	
		// summation of p(play|leave)p(leave)        
		//p(play|leave) = 1 if the played tiles has matched the simulated played tiles
		priority_queue<pair<int, string>> rankedTiles;		
		int probLeave, probLeaveMax;
		bool firstLevel = 1;

		vector<pair<int, string>>ans;
		vector <string>simulation;
		for (int i = 0; i < remainingTiles.size(); i++) {
			simulation = simulate.simulatePlay_TEST(tilesPlayed + remainingTiles[i]);

			if (simulation[0] == tilesPlayed) {

				probLeave = drawFromBag(remainingTiles[i], charInBag);

				rankedTiles.push({ probLeave,remainingTiles[i] });
			}
		}

		if (!rankedTiles.empty())
			probLeaveMax = rankedTiles.top().first;

		while (!rankedTiles.empty()) {
			if (rankedTiles.top().first == probLeaveMax) {
				if (!firstLevel  && ans.size() >= maxTopRackRemained)
					break;
				ans.push_back({ rankedTiles.top().first,rankedTiles.top().second });
				rankedTiles.pop();
			}
			else {
				if (ans.size() >= maxTopRackRemained)
					break;
				firstLevel = 0;
				probLeaveMax = rankedTiles.top().first;

			}

		}

		return ans;
	}


	/*
	* ranks the possible sub racks to refill the played tiles the opponent had played
	*
	* @param estimatedNonPlayed	the estimated non played tiles of the oppnent racks to be
	*							subtracted from the total unseen tiles
	*
	* @param bag	all the unseen tiles (all tiles - (tiles on board + my rack's tiles )
	*				(a copy from the bag to be modified)
	*
	* @param wordsFreqInBag	the frequency of all the chars in the bag (a copy from the charInBag to be modified )
	*
	* @return a ranked sub racks to refill the opponent rack (ranked pased on their probabilty )
	*/
	vector<pair<int, string>>estimateRefillRack(string estimatedNonPlayed, string bag, vector<int> wordsFreqInBag) {
		priority_queue<pair<int, string>> rankedTiles;
		vector<pair<int, string>> ans;
		int probLeave, probLeaveMax = 0;
		bool firstLevel = 1;


		for (int i = 0; i < estimatedNonPlayed.size(); i++) {

			wordsFreqInBag[estimatedNonPlayed[i] - firstIdx]--;

			int index = bag.find(estimatedNonPlayed[i]);		// remove the char from the bag string as well 
			bag.erase(index, 1);

		}


		remainingTiles.clear();

		constructPossibleLeaves(bag, "", 0);
		for (int i = 0; i < remainingTiles.size(); i++) {
			probLeave = drawFromBag(remainingTiles[i], wordsFreqInBag);
			rankedTiles.push({ probLeave,remainingTiles[i] });
		}

		if (!rankedTiles.empty())
			probLeaveMax = rankedTiles.top().first;

		while (!rankedTiles.empty()) {
			if (rankedTiles.top().first == probLeaveMax) {
				if (!firstLevel  && ans.size() >= maxTopRackRefilled)
					break;
				ans.push_back({ rankedTiles.top().first,rankedTiles.top().second });
				rankedTiles.pop();
			}
			else {
				if (ans.size() >= maxTopRackRefilled)
					break;
				probLeaveMax = rankedTiles.top().first;
				firstLevel = 0;


			}
		}


		return ans;

	}


	/*
	* calculats the probability of a leave to draw from the bag
	*
	* @param leave	the leave to be drawn
	*
	* @param charFreq	the frequency of each char in the bag
	*
	* @return	the probability of that leave drawn from the bag
	*/

	int drawFromBag(string leave, vector<int> charFreq) {

		vector<pair<char, int>>charsRemaining = countChars(leave);
		int probLeave = 1;

		for (int i = 0; i < charsRemaining.size(); i++) {
			// nCr[the number of occurence of a char in the bag =X ][the number of chars we will draw from X]
			probLeave *= nCr[charFreq[charsRemaining[i].first - firstIdx]][charsRemaining[i].second];
		}

		return probLeave;
	}

	/*
	* counts the occurence number of each char for the given string
	*
	* @param x	the string to count the number of occurence of each char
	*
	* @return	a vector of pairs of unique chars and the number of occurence of each char in that string x
	*/

	vector<pair<char, int>> countChars(string x) {

		char cChar;				// temp char for comparing 
		int sum;
		vector<pair<char, int>> result;
		if (x.size() != 0) {
			cChar = x[0];
			sum = 0;
			for (int i = 0; i < x.size(); i++) {
				if (cChar == x[i]) {
					sum++;
				}
				else
				{
					result.push_back({ cChar, sum });
					sum = 0;
					cChar = x[i];
					i--;
				}
			}
			result.push_back({ cChar, sum });
		}
		return result;
	}

	/*
	* fills the combinations as 3C2 is represented as C[3][2]
	* in our project we will not use more than 7 number to select from
	*/
	void fillCombinations(int n = 8) {

		for (int i = 0; i<n; i++)
			for (int j = 0; j < n; j++)
				nCr[i][j] = 0;

		for (int i = nCr[0][0] = 1; i <= n; ++i)
			for (int j = nCr[i][0] = 1; j <= i; ++j)
				nCr[i][j] = nCr[i - 1][j] + nCr[i - 1][j - 1];
	}


	/*
	* generates all possible non dublicate strings that can form from a big string
	* (generates all the combinations)
	*
	* @param myBag	the total bag to make our combination from
	*
	* @param rack	the string that will concatenate it recurrency
	*
	* @param idx	the index of the bag that we still in it
	*/

	void constructPossibleLeaves(rack myBag, rack rack, int idx = 0) {
		if (rack.size() > rackSizeCombination)
			return;
		if (rack.size() == rackSizeCombination) {  // find a possible rack
			remainingTiles.push_back(rack.toString());
			return;
		}
		if (idx >= myBag.size()) {
			return;
		}

		int	cnt = 0;

		while (cnt==0&& idx<myBag.size()){
			cnt= myBag[idx++];
		}

		// while (idx + cnt < myBag.size() && myBag[idx] == myBag[idx + cnt]) {   // count the number of dublicate of that char in the bag
		// 	cnt++;
		// }

		for (int i = 0; i <= cnt; ++i) {
			constructPossibleLeaves(myBag, rack, idx + 1);	// take all the combination of that no of char 
			rack[myBag[idx]++]; // get char 
		}
	}

};
