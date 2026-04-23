/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */

	template<
		class Key,
		class T,
		class Hash = std::hash<Key>,
		class Equal = std::equal_to<Key>
	> class linked_hashmap {
	private:
		struct Node {
			pair<const Key, T> kv;
			Node *prev;
			Node *next;
			Node *next_bucket;
			Node(const Key &k, const T &v) : kv(k, v), prev(nullptr), next(nullptr), next_bucket(nullptr) {}
		};
		Node **buckets = nullptr;
		std::size_t bucket_count = 0;
		std::size_t elem_count = 0;
		Node *head = nullptr;
		Node *tail = nullptr;
		Hash hasher;
		Equal equaler;
		static constexpr double load_factor = 0.75;
		std::size_t threshold() const { return (std::size_t)(bucket_count * load_factor); }
		std::size_t index_for(const Key &k) const { return bucket_count ? (std::size_t)(hasher(k) % bucket_count) : 0; }
		void attach_node(Node *n) {
			if (!head) head = tail = n; else { tail->next = n; n->prev = tail; tail = n; }
		}
		void bucket_insert(Node *n) {
			std::size_t idx = index_for(n->kv.first);
			n->next_bucket = buckets[idx];
			buckets[idx] = n;
		}
		void rehash(std::size_t new_count) {
			Node **new_b = new Node*[new_count];
			for (std::size_t i = 0; i < new_count; ++i) new_b[i] = nullptr;
			Node *cur = head;
			while (cur) {
				cur->next_bucket = nullptr;
				std::size_t idx = (std::size_t)(hasher(cur->kv.first) % new_count);
				cur->next_bucket = new_b[idx];
				new_b[idx] = cur;
				cur = cur->next;
			}
			delete [] buckets;
			buckets = new_b;
			bucket_count = new_count;
		}
		Node *find_node(const Key &key) const {
			if (!bucket_count) return nullptr;
			std::size_t idx = index_for(key);
			for (Node *p = buckets[idx]; p; p = p->next_bucket) {
				if (equaler(p->kv.first, key)) return p;
			}
			return nullptr;
		}
		void erase_node(Node *n) {
			if (n->prev) n->prev->next = n->next; else head = n->next;
			if (n->next) n->next->prev = n->prev; else tail = n->prev;
			std::size_t idx = index_for(n->kv.first);
			Node *p = buckets[idx], *prevb = nullptr;
			while (p) {
				if (p == n) {
					if (prevb) prevb->next_bucket = p->next_bucket; else buckets[idx] = p->next_bucket;
					break;
				}
				prevb = p; p = p->next_bucket;
			}
			delete n; --elem_count;
		}
	public:
		typedef pair<const Key, T> value_type;

		class const_iterator;
		class iterator {
		private:
			linked_hashmap *owner = nullptr;
			Node *ptr = nullptr;
			struct __iterator_category_tag {};
			friend class linked_hashmap;
		public:
			using difference_type = std::ptrdiff_t;
			using value_type = typename linked_hashmap::value_type;
			using pointer = value_type*;
			using reference = value_type&;
			using iterator_category = __iterator_category_tag;

			iterator() {}
			iterator(linked_hashmap *o, Node *p) : owner(o), ptr(p) {}
			iterator(const iterator &other) : owner(other.owner), ptr(other.ptr) {}
			iterator operator++(int) {
				if (!owner || !ptr) throw invalid_iterator();
				iterator tmp(*this);
				ptr = ptr->next;
				return tmp;
			}
			iterator & operator++() {
				if (!owner || !ptr) throw invalid_iterator();
				ptr = ptr->next;
				return *this;
			}
			iterator operator--(int) {
				iterator tmp(*this);
				if (!owner) throw invalid_iterator();
				if (!ptr) { if (!owner->tail) throw invalid_iterator(); ptr = owner->tail; return tmp; }
				if (!ptr->prev) throw invalid_iterator();
				ptr = ptr->prev;
				return tmp;
			}
			iterator & operator--() {
				if (!owner) throw invalid_iterator();
				if (!ptr) { if (!owner->tail) throw invalid_iterator(); ptr = owner->tail; return *this; }
				if (!ptr->prev) throw invalid_iterator();
				ptr = ptr->prev;
				return *this;
			}
			value_type & operator*() const { if (!ptr) throw invalid_iterator(); return ptr->kv; }
			bool operator==(const iterator &rhs) const { return owner == rhs.owner && ptr == rhs.ptr; }
			bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && ptr == rhs.ptr; }
			bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
			bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
			value_type* operator->() const noexcept { return ptr ? &ptr->kv : nullptr; }
		};

		class const_iterator {
		private:
			const linked_hashmap *owner = nullptr;
			const Node *ptr = nullptr;
			struct __iterator_category_tag {};
			friend class linked_hashmap;
		public:
			using difference_type = std::ptrdiff_t;
			using value_type = typename linked_hashmap::value_type;
			using pointer = const value_type*;
			using reference = const value_type&;
			using iterator_category = __iterator_category_tag;
			const_iterator() {}
			const_iterator(const linked_hashmap *o, const Node *p) : owner(o), ptr(p) {}
			const_iterator(const const_iterator &other) : owner(other.owner), ptr(other.ptr) {}
			const_iterator(const iterator &other) : owner(other.owner), ptr(other.ptr) {}
			const_iterator operator++(int) {
				if (!owner || !ptr) throw invalid_iterator();
				const_iterator tmp(*this);
				ptr = ptr->next;
				return tmp;
			}
			const_iterator & operator++() {
				if (!owner || !ptr) throw invalid_iterator();
				ptr = ptr->next;
				return *this;
			}
			const_iterator operator--(int) {
				const_iterator tmp(*this);
				if (!owner) throw invalid_iterator();
				if (!ptr) { if (!owner->tail) throw invalid_iterator(); ptr = owner->tail; return tmp; }
				if (!ptr->prev) throw invalid_iterator();
				ptr = ptr->prev;
				return tmp;
			}
			const_iterator & operator--() {
				if (!owner) throw invalid_iterator();
				if (!ptr) { if (!owner->tail) throw invalid_iterator(); ptr = owner->tail; return *this; }
				if (!ptr->prev) throw invalid_iterator();
				ptr = ptr->prev;
				return *this;
			}
			const value_type & operator*() const { if (!ptr) throw invalid_iterator(); return ptr->kv; }
			bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && ptr == rhs.ptr; }
			bool operator==(const iterator &rhs) const { return owner == rhs.owner && ptr == rhs.ptr; }
			bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
			bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
			const value_type* operator->() const noexcept { return ptr ? &ptr->kv : nullptr; }
		};

		linked_hashmap() {
			bucket_count = 16;
			buckets = new Node*[bucket_count];
			for (std::size_t i = 0; i < bucket_count; ++i) buckets[i] = nullptr;
		}
		linked_hashmap(const linked_hashmap &other) {
			bucket_count = other.bucket_count ? other.bucket_count : 16;
			buckets = new Node*[bucket_count];
			for (std::size_t i = 0; i < bucket_count; ++i) buckets[i] = nullptr;
			head = tail = nullptr; elem_count = 0;
			for (Node *p = other.head; p; p = p->next) {
				insert(p->kv);
			}
		}

		linked_hashmap & operator=(const linked_hashmap &other) {
			if (this == &other) return *this;
			clear();
			delete [] buckets;
			bucket_count = other.bucket_count ? other.bucket_count : 16;
			buckets = new Node*[bucket_count];
			for (std::size_t i = 0; i < bucket_count; ++i) buckets[i] = nullptr;
			head = tail = nullptr; elem_count = 0;
			for (Node *p = other.head; p; p = p->next) insert(p->kv);
			return *this;
		}

		~linked_hashmap() { clear(); delete [] buckets; }

		T & at(const Key &key) {
			Node *n = find_node(key);
			if (!n) throw index_out_of_bound();
			return n->kv.second;
		}
		const T & at(const Key &key) const {
			Node *n = find_node(key);
			if (!n) throw index_out_of_bound();
			return n->kv.second;
		}

		T & operator[](const Key &key) {
			Node *n = find_node(key);
			if (n) return n->kv.second;
			Node *nn = new Node(key, T());
			attach_node(nn);
			if (bucket_count == 0) { bucket_count = 16; buckets = new Node*[bucket_count]; for (std::size_t i=0;i<bucket_count;++i) buckets[i]=nullptr; }
			bucket_insert(nn);
			++elem_count;
			if (elem_count > threshold()) rehash(bucket_count * 2);
			return nn->kv.second;
		}

		const T & operator[](const Key &key) const {
			Node *n = find_node(key);
			if (!n) throw index_out_of_bound();
			return n->kv.second;
		}

		iterator begin() { return iterator(this, head ? head : nullptr); }
		const_iterator cbegin() const { return const_iterator(this, head ? head : nullptr); }

		iterator end() { return iterator(this, nullptr); }
		const_iterator cend() const { return const_iterator(this, nullptr); }

		bool empty() const { return elem_count == 0; }

		size_t size() const { return elem_count; }

		void clear() {
			Node *cur = head;
			while (cur) { Node *nxt = cur->next; delete cur; cur = nxt; }
			head = tail = nullptr;
			for (std::size_t i = 0; i < bucket_count; ++i) buckets[i] = nullptr;
			elem_count = 0;
		}

		pair<iterator, bool> insert(const value_type &value) {
			Node *n = find_node(value.first);
			if (n) return pair<iterator, bool>(iterator(this, n), false);
			Node *nn = new Node(value.first, value.second);
			attach_node(nn);
			bucket_insert(nn);
			++elem_count;
			if (elem_count > threshold()) rehash(bucket_count * 2);
			return pair<iterator, bool>(iterator(this, nn), true);
		}

		void erase(iterator pos) {
			if (pos.owner != this || pos.ptr == nullptr) throw invalid_iterator();
			erase_node(pos.ptr);
		}

		size_t count(const Key &key) const { return find_node(key) ? 1 : 0; }

		iterator find(const Key &key) {
			Node *n = find_node(key);
			return iterator(this, n ? n : nullptr);
		}
		const_iterator find(const Key &key) const {
			Node *n = find_node(key);
			return const_iterator(this, n ? n : nullptr);
		}
	};

}

#endif
