#pragma once

#include <boost/iterator/iterator_adaptor.hpp>

template <typename Iterator>
class striding_iterator : public boost::iterator_adaptor<striding_iterator<Iterator>, Iterator> {
	typedef boost::iterator_adaptor<striding_iterator<Iterator>, Iterator> super_t;
	friend class boost::iterator_core_access;
public:
	striding_iterator()
	: super_t()
	, stride(1)
	{}

	striding_iterator(Iterator it, int stride)
	: super_t(it)
	, stride(stride)
	{}

	template<class OtherIterator>
	striding_iterator(striding_iterator<OtherIterator> const& r, typename boost::enable_if_convertible<OtherIterator, Iterator>::type* = 0)
	: super_t(r.base())
	{}

	int stride;

private:
	void advance(typename super_t::difference_type n){
		this->base_reference() += stride * n;
	}

	template <class OtherIterator>
	typename super_t::difference_type distance_to(striding_iterator<OtherIterator> const & that) const {
		int s = that.base() - this->base();
		if(s >= 0) return (s + stride - 1) / stride;
		return (s - stride + 1) / stride;
	}

	void increment() { advance(1); }
	void decrement() { advance(-1); }
};

template <typename Iterator>
striding_iterator<Iterator> strided(Iterator it, int stride = 1){
	return striding_iterator<Iterator>(it, stride);
}
