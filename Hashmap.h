#include <algorithm>
#include <list>
#include <iterator>
#include <iostream>
#include <initializer_list>
#include <vector>
#include <exception>
#include <stdexcept>
#include <cassert>
#include <cstdlib>
#include <functional>

const int C = 4;
const int K = 6;

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:

    using T = std::pair<const KeyType, ValueType>;

    class iterator {
    public:

        iterator() {}

        iterator(std::vector<size_t>* list, std::vector<T>* data, size_t iterator) {
            list_ = list;
            data_ = data;
            iterator_ = iterator;
        }

        T& operator*() const {
            return (*data_)[(*list_)[iterator_]];
        }

        T* operator->() const {
            return &(*data_)[(*list_)[iterator_]];
        }

        iterator& operator++() {
            ++iterator_;
            return *this;
        }

        iterator operator++(int) {
            ++iterator_;
            return iterator(list_, data_, iterator_ - 1);
        }

        bool operator==(const iterator& other) const {
            return list_ == other.list_ && data_ == other.data_ && iterator_ == other.iterator_;
        }

        bool operator!=(const iterator& other) const {
            return list_ != other.list_ || data_ != other.data_ || iterator_ != other.iterator_;
        }

    private:
        std::vector<size_t>* list_;
        std::vector<T>* data_;
        size_t iterator_;
    };

    class const_iterator {
    public:

        const_iterator() {}

        const_iterator(const std::vector<size_t>* list, const std::vector<T>* data, size_t iterator) {
            list_ = list;
            data_ = data;
            iterator_ = iterator;
        }

        const T& operator*() const {
            return (*data_)[(*list_)[iterator_]];
        }

        const T* operator->() const {
            return &(*data_)[(*list_)[iterator_]];
        }

        const_iterator& operator++() {
            ++iterator_;
            return *this;
        }

        const_iterator operator++(int) {
            ++iterator_;
            return const_iterator(list_, data_, iterator_ - 1);
        }

        bool operator==(const const_iterator& other) const {
            return list_ == other.list_ && data_ == other.data_ && iterator_ == other.iterator_;
        }

        bool operator!=(const const_iterator& other) const {
            return list_ != other.list_ || data_ != other.data_ || iterator_ != other.iterator_;
        }

    private:
        const std::vector<size_t>* list_;
        const std::vector<T>* data_;
        size_t iterator_;
    };

    iterator begin() {
        return iterator(&list_, &data_, 0);
    }

    iterator end() {
        return iterator(&list_, &data_, list_.size());
    }

    const_iterator begin() const {
        return const_iterator(&list_, &data_, 0);
    }

    const_iterator end() const {
        return const_iterator(&list_, &data_, list_.size());
    }

    HashMap(const Hash& hasher = Hash()) : hasher_(hasher) {
        resize(1);
    }

    HashMap(const HashMap& hashmap) {
        resize(hashmap.list_.size() * K + 4);
        for (size_t i : hashmap.list_) {
            insert(hashmap.data_[i]);
        }
    }

    HashMap& operator=(const HashMap& hashmap) {
        if (data_ == hashmap.data_ && list_ == hashmap.list_) {
            return *this;
        }
        clear();
        hasher_ = hashmap.hasher_;
        resize(hashmap.list_.size() * K + 4);
        for (size_t i : hashmap.list_) {
            insert(hashmap.data_[i]);
        }
        return *this;
    }

    template<typename iterator>
    HashMap(const iterator& begin, const iterator& end, const Hash& hasher = Hash()) : hasher_(hasher)  {
        size_t size = 0;
        iterator current = begin;
        while (current != end) {
            size++;
            current++;
        }
        resize(size * K + 4);
        current = begin;
        while (current != end) {
            insert(*current);
            current++;
        }
    }

    HashMap(const std::initializer_list<T>& list, const Hash& hasher = Hash()) : hasher_(hasher) {
        resize(list.size() * K + 4);
        for (const T& element : list) {
            insert(element);
        }
    }

    size_t size() const {
        return list_.size();
    }


    bool empty() const {
        return list_.empty();
    }

    Hash hash_function() const {
        return hasher_;
    }

    ValueType& insert(const T& element) {
        if (list_.size() * C * 2 >= size_) {
            resize(list_.size() * K + 4);
        }
        {
            size_t pos = hasher_(element.first);
            pos %= size_;
            while (true) {
                if (map_[pos] == size_) {
                    break;
                }
                if (data_[list_[map_[pos]]].first == element.first) {
                    return data_[list_[map_[pos]]].second;
                }
                pos = (pos + 1 == size_ ? 0 : pos + 1);
            }
        }
        size_t pos = hasher_(element.first);
        pos %= size_;
        size_t dist = 0;
        data_.push_back(element);
        list_.push_back(data_.size() - 1);
        index_.push_back(0);
        size_t it = list_.size() - 1;
        while (true) {
            if (map_[pos] == size_) {
                map_[pos] = it;
                index_[it] = pos;
                distance_[pos] = dist;
                return data_.back().second;
            }
            if (dist >= distance_[pos]) {
                std::swap(dist, distance_[pos]);
                index_[it] = pos;
                std::swap(it, map_[pos]);
            }
            pos = (pos + 1 == size_ ? 0 : pos + 1);
            dist++;
        }
    }

    void erase(const KeyType& key) {
        size_t pos = hasher_(key);
        pos %= size_;
        while (true) {
            if (map_[pos] == size_) {
                break;
            }
            if (data_[list_[map_[pos]]].first == key) {
                size_t i = map_[pos];
                size_t j = list_.size() - 1;
                std::swap(map_[index_[i]], map_[index_[j]]);
                std::swap(list_[i], list_[j]);
                std::swap(index_[i], index_[j]);
                map_[pos] = size_;
                list_.pop_back();
                index_.pop_back();
                while (true) {
                    size_t prev = pos;
                    pos = (pos + 1 == size_ ? 0 : pos + 1);
                    if (map_[pos] == size_ || distance_[pos] == 0) {
                        break;
                    }
                    distance_[pos]--;
                    index_[map_[pos]] = prev;
                    std::swap(map_[prev], map_[pos]);
                    std::swap(distance_[prev], distance_[pos]);
                }
                if (list_.size() * C * 12 < size_) {
                    resize(list_.size() * 6 + 4);
                }
                break;
            }
            pos = (pos + 1 == size_ ? 0 : pos + 1);
        }
    }

    iterator find(const KeyType& key) {
        size_t pos = hasher_(key);
        pos %= size_;
        while (true) {
            if (map_[pos] == size_) {
                return iterator(&list_, &data_, list_.size());
            }
            if (data_[list_[map_[pos]]].first == key) {
                return iterator(&list_, &data_, map_[pos]);
            }
            pos = (pos + 1 == size_ ? 0 : pos + 1);
        }
    }

    const_iterator find(const KeyType& key) const {
        size_t pos = hasher_(key);
        pos %= size_;
        while (true) {
            if (map_[pos] == size_) {
                return const_iterator(&list_, &data_, list_.size());
            }
            if (data_[list_[map_[pos]]].first == key) {
                return const_iterator(&list_, &data_, map_[pos]);
            }
            pos = (pos + 1 == size_ ? 0 : pos + 1);
        }
    }

    ValueType& operator[](const KeyType& key) {
        size_t pos = hasher_(key);
        pos %= size_;
        while (true) {
            if (map_[pos] == size_) {
                return insert(std::make_pair(key, ValueType()));
            }
            if (data_[list_[map_[pos]]].first == key) {
                return data_[list_[map_[pos]]].second;
            }
            pos = (pos + 1 == size_ ? 0 : pos + 1);
        }
    }

    const ValueType& at(const KeyType& key) const {
        size_t pos = hasher_(key);
        pos %= size_;
        while (true) {
            if (map_[pos] == size_) {
                throw std::out_of_range("no such key");
            }
            if (data_[list_[map_[pos]]].first == key) {
                return data_[list_[map_[pos]]].second;
            }
            pos = (pos + 1 == size_ ? 0 : pos + 1);
        }
    }

    void clear() {
        list_.clear();
        resize(1);
    }

private:

    Hash hasher_;
    size_t size_ = 0;
    std::vector<size_t> distance_;
    std::vector<size_t> map_;
    std::vector<size_t> list_;
    std::vector<size_t> index_;
    std::vector<T> data_;

    void resize(size_t n) {
        size_ = n * C;
        auto data = list_;
        list_.clear();
        index_.clear();
        distance_.assign(size_, 0);
        map_.assign(size_, size_);
        for (size_t i : data) {
            insert(data_[i]);
        }
    }

    void print() {
        std::cerr << "print" << std::endl;
        std::cerr << list_.size() << std::endl;
        for (size_t i = 0; i < list_.size(); i++) {
            std::cerr << data_[list_[i]].first << " " << data_[list_[i]].second << " " <<
            index_[i] << " " << list_[i] << std::endl;
        }
        std::cerr << "end" << std::endl;
    }
};
