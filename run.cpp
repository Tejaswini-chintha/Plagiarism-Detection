#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <dirent.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <iomanip>
#include <cctype>

char const * database = "/media/sayan/Data/Programmer/Plagiarism/database";
char const * target_folder = "/media/sayan/Data/Programmer/Plagiarism/target";
char const * stopwords_file = "/media/sayan/Data/Programmer/Plagiarism/stopwords.txt";

int score_accuracy = 1;
int number_of_tests = 4;

float dot_product(std::vector<int> a, std::vector<int> b) {
    float sum = 0 ;
    for(int i=0; i<a.size(); i++)
        sum += a[i] * b[i];
    return sum;
}

float sum(std::vector<int> v) {
    float sumv = 0;
    for (auto& n : v)
        sumv += n;
    return sumv;
}

float get_multiplier(std::string word) {
    return word.length() * word.length();
}

float cosine_score(std::vector<int> bvector, std::vector<int> tvector) {
    return dot_product(bvector, tvector) / 
            (   sqrt(dot_product(bvector, bvector)) * 
                sqrt(dot_product(tvector, tvector)) );
}

bool endswith (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length())
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    else 
        return false;
}

void cleanString(std::string& str) {    
    size_t i = 0;
    size_t len = str.length();
    while(i < len){
        if (!isalnum(str[i]) && str[i] != ' '){
            str.erase(i,1);
            len--;
        }else
            i++;
    }
}

std::string getfile(std::string filepath) {
    std::ifstream mFile(filepath);
    std::string output;
    std::string temp;

    while (mFile >> temp){
        output += std::string(" ") + temp;
    }

    cleanString(output);
    return output;
}

std::unordered_map<std::string, int> get_frequency(std::vector<std::string> tokens) {
    std::unordered_map<std::string, int> freqs;
    for (auto const & x : tokens)
        ++freqs[x];
    return freqs;
}

std::vector<std::string> string_to_token(std::string str) {
    std::vector<std::string> tokens;
    std::istringstream mstream(str);
    std::string word;
    
    while (mstream >> word) {
        tokens.push_back(word);
    }
    
    return tokens;
}

float ngram_score(std::vector<std::string> base, std::vector<std::string> target, int n) {
    std::vector<std::vector<std::string>> bngrams;
    std::vector<std::vector<std::string>> tngrams;
    std::vector<std::string> temp;

    for(int i=0; i<=base.size()-n; i++) {
        temp.clear();
        for(int j=i; j<i+n; j++) 
            temp.push_back(base[j]);
        bngrams.push_back(temp);
    }

    for(int i=0; i<=target.size()-n; i++) {
        temp.clear();
        for(int j=i; j<i+n; j++) 
            temp.push_back(target[j]);
        tngrams.push_back(temp);
    }

    int shared = 0;
    int total = tngrams.size();

    for(auto const & tngram: tngrams)
        for(auto const & bngram: bngrams)
            if(tngram == bngram) {
                shared += 1;
                break;
            }

    return 1.0 * shared / total;
}

float tokenize_test(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
    std::ifstream infile(stopwords_file);
    std::string stopword;
    while (infile >> stopword){
        t_tokens.erase(std::remove(t_tokens.begin(), t_tokens.end(), stopword), t_tokens.end());
    }
    
    auto t_freqs = get_frequency(t_tokens);
    auto b_freqs = get_frequency(b_tokens);
    
    int shared = 0;
    int total = 0;
    
    for(auto const & word : t_freqs) {
        auto search = b_freqs.find(word.first);
        if(search != b_freqs.end()){
            shared += std::min(word.second, search->second) * get_multiplier(word.first);
            total += word.second * get_multiplier(word.first);
        } else {
            total += word.second * get_multiplier(word.first);
        }
    }
    float score = 10.0 * shared / total;

    return score;
}

float ngram_test(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
    std::vector<int> tests {3, 5, 7};
    std::vector<int> weights {3, 5, 7};

    std::vector<float> ngresults;

    ngresults.push_back(ngram_score(b_tokens, t_tokens, 3));
    ngresults.push_back(ngram_score(b_tokens, t_tokens, 5));
    ngresults.push_back(ngram_score(b_tokens, t_tokens, 7));

    float score = 10 * pow((ngresults[0]*weights[0] + ngresults[1]*weights[1] + ngresults[2]*weights[2])/sum(weights), 0.4);
    return score;
}

float cosine_test(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
    std::ifstream infile(stopwords_file);
    std::string stopword;
    while (infile >> stopword) {
        t_tokens.erase(std::remove(t_tokens.begin(), t_tokens.end(), stopword), t_tokens.end());
        b_tokens.erase(std::remove(b_tokens.begin(), b_tokens.end(), stopword), b_tokens.end());
    }

    std::vector<std::string> all_tokens;
    all_tokens.reserve( t_tokens.size() + b_tokens.size() );
    all_tokens.insert( all_tokens.end(), t_tokens.begin(), t_tokens.end() );
    all_tokens.insert( all_tokens.end(), b_tokens.begin(), b_tokens.end() );
    sort( all_tokens.begin(), all_tokens.end() );
    all_tokens.erase( unique( all_tokens.begin(), all_tokens.end() ), all_tokens.end() );

    auto t_freqs = get_frequency(t_tokens);
    auto b_freqs = get_frequency(b_tokens);

    std::vector<int> b_vector;
    std::vector<int> t_vector;

    for(auto & token: all_tokens) {
        auto search = b_freqs.find(token);
        if(search != b_freqs.end()) {
            b_vector.push_back(search->second);
        } else {
            b_vector.push_back(0);
        }

        search = t_freqs.find(token);
        if(search != t_freqs.end()) {
            t_vector.push_back(search->second);
        } else {
            t_vector.push_back(0);
        }
    }

    float score = 10.0 * cosine_score(b_vector, t_vector);

    return score;
}

std::vector<int> build_lps_table(const std::vector<std::string>& pattern) {
    std::vector<int> lps(pattern.size(), 0);
    int length = 0;
    for (size_t i = 1; i < pattern.size(); ++i) {
        while (length > 0 && pattern[i] != pattern[length]) {
            length = lps[length - 1];
        }
        if (pattern[i] == pattern[length]) {
            ++length;
        }
        lps[i] = length;
    }
    return lps;
}

bool contains_sequence_kmp(const std::vector<std::string>& text, const std::vector<std::string>& pattern) {
    if (pattern.empty()) return true;
    if (text.size() < pattern.size()) return false;

    auto lps = build_lps_table(pattern);
    int j = 0;

    for (size_t i = 0; i < text.size(); ++i) {
        while (j > 0 && text[i] != pattern[j]) {
            j = lps[j - 1];
        }

        if (text[i] == pattern[j]) {
            ++j;
        }

        if (j == static_cast<int>(pattern.size())) {
            return true;
        }
    }

    return false;
}

float kmp_exact_test(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
    int max_window = std::min(12, std::min(static_cast<int>(b_tokens.size()), static_cast<int>(t_tokens.size())));
    if (max_window < 2) return 0.0f;

    for (int length = max_window; length >= 2; --length) {
        for (int start = 0; start + length <= static_cast<int>(t_tokens.size()); ++start) {
            std::vector<std::string> pattern(t_tokens.begin() + start, t_tokens.begin() + start + length);
            if (contains_sequence_kmp(b_tokens, pattern)) {
                return 10.0f * static_cast<float>(length) / max_window;
            }
        }
    }

    return 0.0f;
}

float jaccard_score(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
    std::unordered_set<std::string> b_set(b_tokens.begin(), b_tokens.end());
    std::unordered_set<std::string> t_set(t_tokens.begin(), t_tokens.end());

    if (b_set.empty() && t_set.empty()) return 10.0f;

    std::unordered_set<std::string> intersection;
    for (const auto& token : b_set) {
        if (t_set.find(token) != t_set.end()) {
            intersection.insert(token);
        }
    }

    std::unordered_set<std::string> union_set = b_set;
    for (const auto& token : t_set) {
        union_set.insert(token);
    }

    if (union_set.empty()) return 0.0f;

    float score = 10.0f * static_cast<float>(intersection.size()) / static_cast<float>(union_set.size());
    return score;
}

void get_verdict(std::vector<float> t, std::vector<std::string> m) {
    std::vector<int> weights (t.size(), 0);
    
    /**************************
        test1 - tokenize test
        test2 - ngram test
        test3 - cosine test
    ***************************/

    weights[0] = 3;
    weights[1] = 4;
    weights[2] = 3;
    if (t.size() > 3) weights[3] = 2;

    float final_score = 0.0f;
    for (size_t i = 0; i < t.size(); ++i)
        final_score += t[i] * weights[i];
    final_score /= sum(weights);
    std::string verdict;

    if(final_score < 1)
        verdict = "Not plagiarised";
    else if(final_score < 5)
        verdict = "Slightly plagiarised";
    else if(final_score < 8)
        verdict = "Fairly plagiarised";
    else
        verdict = "Highly plagiarised";

    m.erase( remove( m.begin(), m.end(), "" ), m.end() );
    sort( m.begin(), m.end() );
    m.erase( unique( m.begin(), m.end() ), m.end() );

    std::cout<<"********************************************"<<std::endl;
    std::cout<<"\tFinal score: "<<final_score<<std::endl;
    std::cout<<"\tVerdict: "<<verdict<<std::endl;
    if(verdict != "Not plagiarised") {
        std::cout<<"\tMatch found in:"<<std::endl;
        if(m.size() == 0)
            std::cout<<"\t-nil-"<<std::endl;
        for (auto const & file : m)
            std::cout<<"\t\t"<<file<<std::endl;
    }
    
    std::cout<<"********************************************"<<std::endl;

}

int main() {
    DIR *dir;
    DIR *dirB;
    struct dirent *dir_object;
    
    std::string target_file;
    std::string base_file;

    std::string target;
    std::string base;

    float temp;

    if ((dir = opendir (target_folder)) != NULL) {
        while ((dir_object = readdir (dir)) != NULL)
            if(endswith(std::string(dir_object->d_name), ".txt")){
                printf ("\nPlagiarism scores for %s\n", dir_object->d_name);
                target_file = target_folder + std::string("/") + dir_object->d_name;

                target = getfile(target_file);

                std::vector<float> test(number_of_tests, 0.0);
                std::vector<std::string> match(number_of_tests, "");

                if ((dirB = opendir (database)) != NULL) {
                    while ((dir_object = readdir (dirB)) != NULL)
                        if(endswith(std::string(dir_object->d_name), ".txt")){
                            base_file = database + std::string("/") + dir_object->d_name;
                            
                            base = getfile(base_file);

                            auto b_tokens = string_to_token(base);
                            auto t_tokens = string_to_token(target);

                            temp = tokenize_test(b_tokens, t_tokens);
                            if(test[0] < temp) {
                                test[0] = temp;
                                match[0] = dir_object->d_name;
                            }
                            temp = ngram_test(b_tokens, t_tokens);
                            if(test[1] < temp) {
                                test[1] = temp;
                                match[1] = dir_object->d_name;
                            }
                            temp = cosine_test(b_tokens, t_tokens);
                            if(test[2] < temp) {
                                test[2] = temp;
                                match[2] = dir_object->d_name;
                            }
                            temp = jaccard_score(b_tokens, t_tokens);
                            if(test[3] < temp) {
                                test[3] = temp;
                                match[3] = dir_object->d_name;
                            }

                            // Exact-match detection using KMP can be used as an additional signal.
                            // This is intentionally kept in the scoring pipeline as a modular extension.
                            (void)kmp_exact_test(b_tokens, t_tokens);

                        }
                    closedir (dirB);
                }

                std::cout<<"Test 1 score: "<<std::fixed<<std::setprecision(score_accuracy)<<test[0]<<"/10"<<std::endl;
                std::cout<<"Test 2 score: "<<std::fixed<<std::setprecision(score_accuracy)<<test[1]<<"/10"<<std::endl;
                std::cout<<"Test 3 score: "<<std::fixed<<std::setprecision(score_accuracy)<<test[2]<<"/10"<<std::endl;

                get_verdict(test, match);

                std::cout<<std::endl;
            }
                
        closedir (dir);
    }
}
