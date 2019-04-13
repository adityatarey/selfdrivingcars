import heapq
class PriorityQ:
    def __init__(self):
        self.member = []
    
    def empty(self):
        return len(self.member) == 0
    
    def put(self, item, priority):
        heapq.heappush(self.member, (priority, item))
    
    def get(self):
        return heapq.heappop(self.member)[1]

def shortest_path(M, start, goal):
    boundary = PriorityQ()
    boundary.put(start, 0)
    cameFrom = {}
    cost = {}
    cameFrom[start] = None
    cost[start] = 0
    
    while not boundary.empty():
        current = boundary.get()
        
        if current == goal:
            return final_path(cameFrom, start, goal)
        
        for neighbour in M.roads[current]:
            recentCost = cost[current] + eD(M,current, neighbour)
            if neighbour not in cost or recentCost < cost[neighbour]:
                cost[neighbour] = recentCost
                priority = recentCost + eD(M,goal, neighbour)
                boundary.put(neighbour, priority)
                cameFrom[neighbour] = current
    
    return cameFrom, cost

def eD(M,spot, des):
    mapUse  = M
    spotY=mapUse.intersections[spot][0]
    desY =mapUse.intersections[des][0]
    diffY=desY-spotY
    
    spotX=mapUse.intersections[spot][1]
    desX =mapUse.intersections[des][1]
    diffX=desX-spotX
    estimated_distance = (diffX**2+diffY**2)**(0.5)
    return estimated_distance

def final_path(cameFrom, start, goal):
    current = goal
    path = []
    while current != start:
        path.append(current)
        current = cameFrom[current]
    path.append(start)
    path.reverse()
    return path