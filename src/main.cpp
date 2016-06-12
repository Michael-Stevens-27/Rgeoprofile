
#include <Rcpp.h>
#include "main.h"
#include "misc.h"
#include "probability.h"
#include "Hungarian.h"

using namespace std;


//------------------------------------------------
// run main MCMC.
// [[Rcpp::export]]
Rcpp::List C_geoMCMC(Rcpp::List data, Rcpp::List params) {
    
    //read in input
    vector<double> data_x = as<vector<double> >(data["longitude"]);
    vector<double> data_y = as<vector<double> >(data["latitude"]);
    int n = data_x.size();
    
    Rcpp::List params_model = params["model"];
    Rcpp::List params_MCMC = params["MCMC"];
    Rcpp::List params_output = params["output"];
    
    double sigma_alpha = as<double>(params_model["sigma_alpha"]);
    double sigma_beta = as<double>(params_model["sigma_beta"]);
    //double sigma = 0.008;
    //double sigma2 = sigma*sigma;
    double priorMean_x = as<double>(params_model["priorMean_longitude"]);
    double priorMean_y = as<double>(params_model["priorMean_latitude"]);
    double tau = as<double>(params_model["priorSD"]);
    double tau2 = tau*tau;
    double alpha_shape = as<double>(params_model["alpha_shape"]);
    double alpha_rate = as<double>(params_model["alpha_rate"]);
    
    int chains = as<int>(params_MCMC["chains"]);
    int burnin = as<int>(params_MCMC["burnin"]);
    int samples = as<int>(params_MCMC["samples"]);
    int burnin_printConsole = as<int>(params_MCMC["burnin_printConsole"]);
    int samples_printConsole = as<int>(params_MCMC["samples_printConsole"]);
    
    vector<double> x_minMax = as<vector<double> >(params_output["longitude_minMax"]);
    double x_min = x_minMax[0];
    double x_max = x_minMax[1];
    vector<double> y_minMax = as<vector<double> >(params_output["latitude_minMax"]);
    double y_min = y_minMax[0];
    double y_max = y_minMax[1];
    int x_cells = as<int>(params_output["longitude_cells"]);
    int y_cells = as<int>(params_output["latitude_cells"]);
    double x_cellSize = (x_max-x_min)/double(x_cells);
    double y_cellSize = (y_max-y_min)/double(y_cells);
    
    
    //## MCMC: burnin #################################################
    // create objects for burn-in phase of MCMC (multiple chains)
    vector< vector<int> > C_group(chains,vector<int>(n,1));
    vector< vector<int> > C_freqs(chains,vector<int>(2));
    vector< vector<double> > C_sum_x(chains,vector<double>(2));
    vector< vector<double> > C_sum_y(chains,vector<double>(2));
    vector<double> C_sigma2(chains);
    vector<double> C_alpha(chains);
    for (int chain=0; chain<chains; chain++) {
        for (int i=0; i<n; i++) {
            C_freqs[chain][C_group[chain][i]-1] ++;
            C_sum_x[chain][C_group[chain][i]-1] += data_x[i];
            C_sum_y[chain][C_group[chain][i]-1] += data_y[i];
        }
        C_sigma2[chain] = 1.0/rgamma1(sigma_alpha,sigma_beta);
        C_alpha[chain] = rgamma1(1.0,0.01);
    }
    vector< vector<double> > C_postMean_x(chains,vector<double>(2));
    vector< vector<double> > C_postMean_y(chains,vector<double>(2));
    vector< vector<double> > C_postVar(chains,vector<double>(2));
    vector< vector<double> > C_logProbVec(chains,vector<double>(2));
    vector< vector<double> > C_probVec(chains,vector<double>(2));
    vector<int> C_nextGroup(chains,2);
    vector<int> C_uniqueGroups(chains,1);
    double lambda, w;
    
    // objects involved in convergence testing
    vector< vector<double> > C_alphaRunningSum(chains,vector<double>(burnin));
    vector< vector<double> > C_alphaRunningSumSquared(chains,vector<double>(burnin));
    vector<double> C_alpha_secondHalf_mean(chains);
    vector<double> C_alpha_secondHalf_sumSquared(chains);
    vector<double> C_alpha_secondHalf_var(chains);
    double GR_W;
    double GR_alpha_grandMean;
    double GR_B;
    double GR_V;
    double GR_R;
    bool convergence_reached = false;
    
    // begin burn-in phase
    if (burnin>0) {
    
    Rcout << "Initiating burn-in phase (" << chains << " chains)\n";
    R_FlushConsole(); R_ProcessEvents();
    for (int rep=0; rep<burnin; rep++) {
        
        // print iteration
        if ((rep+1)%burnin_printConsole==0) {
            Rcout << "  iteration: " << rep+1 << "\n";
            R_FlushConsole(); R_ProcessEvents();
        }
        
        // loop over chains
        for (int chain=0; chain<chains; chain++) {
            
            // update all group allocations
            for (int i=0; i<n; i++) {
                updateGroup(i, n, data_x, data_y, C_group[chain], C_freqs[chain], C_sum_x[chain], C_sum_y[chain], C_nextGroup[chain], C_uniqueGroups[chain], C_postVar[chain], C_postMean_x[chain], C_postMean_y[chain], priorMean_x, priorMean_y, C_sigma2[chain], tau2, C_probVec[chain], C_logProbVec[chain], C_alpha[chain]);
            }
            
            // update sigma2
            
            
            // update alpha
            lambda = rbeta1(C_alpha[chain]+1.0,n);
            w = n/(n + (alpha_shape+C_uniqueGroups[chain]-1)/(alpha_rate-log(lambda)) );
            if (runif1()<w) {
                C_alpha[chain] = rgamma1(alpha_shape+C_uniqueGroups[chain]-1, alpha_rate-log(lambda));
            } else {
                C_alpha[chain] = rgamma1(alpha_shape+C_uniqueGroups[chain], alpha_rate-log(lambda));
            }
            
            // GR diagnostic elements
            if (rep==0) {
                // add current alpha to running sums
                C_alphaRunningSum[chain][rep] = C_alpha[chain];
                C_alphaRunningSumSquared[chain][rep] = C_alpha[chain]*C_alpha[chain];
            } else {
                // add current alpha to running sums
                C_alphaRunningSum[chain][rep] = C_alphaRunningSum[chain][rep-1] + C_alpha[chain];
                C_alphaRunningSumSquared[chain][rep] = C_alphaRunningSumSquared[chain][rep-1] + C_alpha[chain]*C_alpha[chain];
                
                // calculate variance of second half of alpha estimates
                C_alpha_secondHalf_mean[chain] = (C_alphaRunningSum[chain][rep] - C_alphaRunningSum[chain][floor(rep/2.0)])/ceil(rep/2.0);
                C_alpha_secondHalf_sumSquared[chain] = C_alphaRunningSumSquared[chain][rep] - C_alphaRunningSumSquared[chain][floor(rep/2.0)];
                C_alpha_secondHalf_var[chain] = (C_alpha_secondHalf_sumSquared[chain]-ceil(rep/2.0)*C_alpha_secondHalf_mean[chain]*C_alpha_secondHalf_mean[chain])/ceil(rep/2.0-1.0);
            }
            
        } // loop over chains
        
        // GR diagnostic calculation
        GR_W = mean(C_alpha_secondHalf_var);
        GR_alpha_grandMean = mean(C_alpha_secondHalf_mean);
        GR_B = 0;
        for (int chain=0; chain<chains; chain++) {
            GR_B += (C_alpha_secondHalf_mean[chain]-GR_alpha_grandMean)*(C_alpha_secondHalf_mean[chain]-GR_alpha_grandMean);
        }
        GR_B *= ceil(rep/2.0)/double(chains-1);
        GR_V = (1-1/ceil(rep/2.0))*GR_W + GR_B/ceil(rep/2.0);
        GR_R = sqrt(GR_V/GR_W);
        
        
        if ((rep+1)>=100 && GR_R<1.1 && convergence_reached==false) {
            Rcout << "    convergence at the GR=1.1 level reached within " << rep+1 << " iterations\n";
            R_FlushConsole(); R_ProcessEvents();
            convergence_reached = true;
        }
        
        
    } // loop over burnin iterations
    
    // print final convergence score
    if (burnin>3) {
        Rcout << "    final GR statistic: GR=" << GR_R << "\n\n";
        R_FlushConsole(); R_ProcessEvents();
    } else {
        Rcout << "    unable to compute GR diagnostic on <4 burn-in iterations\n\n";
        R_FlushConsole(); R_ProcessEvents();
    }
    
    // if burnin>0
    } else {
        Rcout << "No burn-in phase selected\n\n";
        R_FlushConsole(); R_ProcessEvents();
    }
    
    
    //## MCMC: sampling #################################################
    // create objects for sampling phase of MCMC
    // start by reordering burn-in group to be increasing
    vector<int> group(n);
    vector<int> burnin_reorder(C_freqs[0].size());
    int index1 = 0;
    for (int i=0; i<n; i++) {
        if (burnin_reorder[C_group[0][i]-1]==0) {
            index1 ++;
            group[i] = index1;
            burnin_reorder[C_group[0][i]-1] = index1;
        } else {
            group[i] = burnin_reorder[C_group[0][i]-1];
        }
    }
    
    vector<int> freqs(index1+1);
    vector<double> sum_x(index1+1);
    vector<double> sum_y(index1+1);
    for (int i=0; i<n; i++) {
        freqs[group[i]-1] ++;
        sum_x[group[i]-1] += data_x[i];
        sum_y[group[i]-1] += data_y[i];
    }
    vector<double> postMean_x(index1+1);
    vector<double> postMean_y(index1+1);
    vector<double> postVar(index1+1);
    vector<double> logProbVec(index1+1);
    vector<double> probVec(index1+1);
    int nextGroup = index1+1;
    int uniqueGroups = index1;
    double sigma2 = C_sigma2[0];
    double sigma_postAlpha;
    double sigma_postBeta;
    double alpha = C_alpha[0];
    
    
    // create objects for dealing with label switching
    vector< vector<int> > groupMat(n, vector<int>(1));
    vector<int> bestPerm;
    vector<int> bestPermOrder(1);
    vector<int> group_reorder(n);
    vector<int> freqs_reorder(2);
    vector<double> sum_x_reorder(2);
    vector<double> sum_y_reorder(2);
    
    // create objects for storing results
    vector<double> sigma_store(samples);
    vector<double> alpha_store(samples);
    vector< vector<double> > geoSurface(y_cells, vector<double>(x_cells));
    
    
    // MCMC: sampling
    print("Initiating sampling phase");
    for (int rep=0; rep<samples; rep++) {
     
        // print iteration
        if ((rep+1)%samples_printConsole==0) {
            Rcout << "  iteration: " << rep+1 << "\n";
            R_FlushConsole();
            R_ProcessEvents();
        }
        
        // update all group allocations
        for (int i=0; i<n; i++) {
            updateGroup(i, n, data_x, data_y, group, freqs, sum_x, sum_y, nextGroup, uniqueGroups, postVar, postMean_x, postMean_y, priorMean_x, priorMean_y, sigma2, tau2, probVec, logProbVec, alpha);
        }
        
        // solve label switching problem
        solveLabelSwitching(n, group, groupMat, bestPerm, bestPermOrder, group_reorder, freqs, sum_x, sum_y, freqs_reorder, sum_x_reorder, sum_y_reorder, nextGroup);
        
        // update sigma2
        sigma_postAlpha = sigma_alpha;
        sigma_postBeta = sigma_beta;
        
        sigma2 = 1.0/rgamma1(sigma_postAlpha,sigma_postBeta);
        
        // update alpha
        lambda = rbeta1(alpha+1.0,n);
        w = n/(n + (alpha_shape+uniqueGroups-1)/(alpha_rate-log(lambda)) );
        if (runif1()<w) {
            alpha = rgamma1(alpha_shape+uniqueGroups-1, alpha_rate-log(lambda));
        } else {
            alpha = rgamma1(alpha_shape+uniqueGroups, alpha_rate-log(lambda));
        }
        
        // draw group means and add to geoSurface
        for (int j=0; j<int(freqs.size()); j++) {
            if (freqs[j]>0) {
                postVar[j] = 1/(double(freqs[j])/sigma2+1/tau2);
                postMean_x[j] = (sum_x[j]/sigma2+priorMean_x/tau2)*postVar[j];
                postMean_y[j] = (sum_y[j]/sigma2+priorMean_y/tau2)*postVar[j];
                double x_draw = rnorm1(postMean_x[j],sqrt(postVar[j]));
                double y_draw = rnorm1(postMean_y[j],sqrt(postVar[j]));
                if (x_draw>=x_min && x_draw<=x_max && y_draw>=y_min && y_draw<=y_max) {
                    geoSurface[floor((y_draw-y_min)/double(y_cellSize))][floor((x_draw-x_min)/double(x_cellSize))] += freqs[j]/(n+alpha);
                }
            }
        }
        
        // store some results
        sigma_store[rep] = sqrt(sigma2);
        alpha_store[rep] = alpha;
     
    } // loop over sampling iterations
    
    // final message
    Rcout << "\nMCMC complete\n";
    R_FlushConsole(); R_ProcessEvents();
    
    // return values
    return Rcpp::List::create(Named("alpha")=alpha_store, Named("allocation")=groupMat, Named("geoSurface")=geoSurface);
}

//------------------------------------------------
// update group allocation
void updateGroup(int &i, int &n, vector<double> &data_x, vector<double> &data_y, vector<int> &group, vector<int> &freqs, vector<double> &sum_x, vector<double> &sum_y, int &nextGroup, int &uniqueGroups, vector<double> &postVar, vector<double> &postMean_x, vector<double> &postMean_y, double &priorMean_x, double &priorMean_y, double &sigma2, double &tau2, vector<double> &probVec, vector<double> &logProbVec, double &alpha) {
    
    // remove point i from all objects
    freqs[group[i]-1] --;
    sum_x[group[i]-1] -= data_x[i];
    sum_y[group[i]-1] -= data_y[i];
    
    // if group is emptied
    if (freqs[group[i]-1]==0) {
        // fix zero values
        sum_x[group[i]-1] = 0;
        sum_y[group[i]-1] = 0;
        
        // reduce unique groups
        uniqueGroups --;
        
        // make this the nextGroup
        nextGroup = group[i];
    }
    
    // recalculate probabilities
    double logProbVec_sum = log(0.0);
    for (int j=0; j<int(freqs.size()); j++) {
        logProbVec[j] = log(0.0);
        if (freqs[j]>0 || j==(nextGroup-1)) {
            postVar[j] = 1/(double(freqs[j])/sigma2+1/tau2);
            postMean_x[j] = (sum_x[j]/sigma2+priorMean_x/tau2)*postVar[j];
            postMean_y[j] = (sum_y[j]/sigma2+priorMean_y/tau2)*postVar[j];
            logProbVec[j] = -log(postVar[j]+sigma2)-0.5/(postVar[j]+sigma2)*((data_x[i]-postMean_x[j])*(data_x[i]-postMean_x[j]) + (data_y[i]-postMean_y[j])*(data_y[i]-postMean_y[j]));
            if (j==(nextGroup-1)) {
                logProbVec[j] += log(alpha);
            } else {
                logProbVec[j] += log(double(freqs[j]));
            }
            logProbVec_sum = logSum(logProbVec_sum,logProbVec[j]);
        }
    }
    for (int j=0; j<int(freqs.size()); j++) {
        probVec[j] = exp(logProbVec[j]-logProbVec_sum);
    }
    
    // sample new group from probVec
    group[i] = sample1(probVec,1.0);
    
    // expand to accommodate new group if needed
    if (group[i]==int(freqs.size())) {
        freqs.push_back(0);
        sum_x.push_back(0);
        sum_y.push_back(0);
        postMean_x.push_back(0);
        postMean_y.push_back(0);
        postVar.push_back(0);
        logProbVec.push_back(0);
        probVec.push_back(0);
    }
    
    // update frequencies etc.
    freqs[group[i]-1] ++;
    sum_x[group[i]-1] += data_x[i];
    sum_y[group[i]-1] += data_y[i];
    
    // find new nextGroup if needed
    if (group[i]==nextGroup) {
        uniqueGroups ++;
        nextGroup = 1;
        for (int j=0; j<int(freqs.size()); j++) {
            if (freqs[j]==0)
                break;
            nextGroup++;
        }
    }
    
}

//------------------------------------------------
// solve label switching problem
void solveLabelSwitching(int &n, vector<int> &group, vector< vector<int> > &groupMat, vector<int> &bestPerm, vector<int> &bestPermOrder, vector<int> &group_reorder, vector<int> &freqs, vector<double> &sum_x, vector<double> &sum_y, vector<int> &freqs_reorder, vector<double> &sum_x_reorder, vector<double> &sum_y_reorder,  int &nextGroup) {
    
    // expand objects if necessary
    while ((freqs.size()-1)>bestPermOrder.size()) {
        for (int j=0; j<n; j++) {
            groupMat[j].push_back(0);
        }
        bestPermOrder.push_back(0);
        freqs_reorder.push_back(0);
        sum_x_reorder.push_back(0);
        sum_y_reorder.push_back(0);
    }
    
    // calculate cost matrix
    vector< vector<double> > costMat(groupMat[0].size(), vector<double>(groupMat[0].size()));
    for (int i=0; i<n; i++) {
        for (int j=0; j<int(costMat.size()); j++) {
            costMat[group[i]-1][j] -= groupMat[i][j];
        }
    }
    
    // calculate bestPerm and bestPermOrder
    bestPerm = hungarian(costMat);
    for (int j=0; j<int(bestPerm.size()); j++) {
        bestPermOrder[bestPerm[j]] = j;
    }
    
    // reorder labels
    for (int i=0; i<n; i++) {
        group_reorder[i] = bestPerm[group[i]-1]+1;
        groupMat[i][group_reorder[i]-1] ++;
    }
    for (int j=0; j<int(bestPermOrder.size()); j++) {
        freqs_reorder[j] = freqs[bestPermOrder[j]];
        sum_x_reorder[j] = sum_x[bestPermOrder[j]];
        sum_y_reorder[j] = sum_y[bestPermOrder[j]];
    }
    group = group_reorder;
    freqs = freqs_reorder;
    sum_x = sum_x_reorder;
    sum_y = sum_y_reorder;
    if (nextGroup<int(freqs.size()))
        nextGroup = bestPerm[nextGroup-1]+1;
    
}
