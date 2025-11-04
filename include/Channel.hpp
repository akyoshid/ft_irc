/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 00:37:05 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/04 00:37:45 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_CHANNEL_HPP_
# define INCLUDE_CHANNEL_HPP_

class Channel {
 public:
    Channel();
    ~Channel();
 private:
    Channel(const Channel& src);  // = delete
    Channel& operator=(const Channel& src);  // = delete
};

#endif
