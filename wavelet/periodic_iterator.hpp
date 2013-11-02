#pragma once

#include <boost/iterator/iterator_adaptor.hpp>

template <typename Iterator>
class periodic_iterator : public boost::iterator_adaptor<periodic_iterator<Iterator>, Iterator> {
	typedef boost::iterator_adaptor<periodic_iterator<Iterator>, Iterator> super_t;
	friend class boost::iterator_core_access;
public:
	periodic_iterator()
	: super_t()
	, end()
	, length(1)
	{}

	periodic_iterator(Iterator begin, Iterator end)
	: super_t(begin)
	, end(end)
	, length(end - begin)
	{}

private:
	void advance(typename super_t::difference_type n){
		this->base_reference() += n;
		if(this->base() >= end) this->base_reference() -= length;
	}

	void increment() { advance(1); }
	void decrement() { advance(-1); }

	Iterator end;
	int length;
};

template <typename Iterator>
periodic_iterator<Iterator> periodic(Iterator begin, Iterator end){
	return periodic_iterator<Iterator>(begin, end);
}
